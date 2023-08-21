//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: buffer serialization/deserialization.
//
// $NoKeywords: $
//===========================================================================//

#include "tier1/bitbuf.h"
#include "mathlib/swap.h"
#include "mathlib/bitvec.h"


//-----------------------------------------------------------------------------
// Write masks
//-----------------------------------------------------------------------------
class CBitWriteMasks
{
public:
	CBitWriteMasks()
	{
		for (unsigned int startbit = 0; startbit < 32; startbit++)
		{
			for (unsigned int nBitsLeft = 0; nBitsLeft < 33; nBitsLeft++)
			{
				unsigned int endbit = startbit + nBitsLeft;
				m_BitWriteMasks[startbit][nBitsLeft] = GetBitForBitnum(int(startbit)) - 1;
				if (endbit < 32)
					m_BitWriteMasks[startbit][nBitsLeft] |= ~(GetBitForBitnum(int(endbit)) - 1);
			}
		}

		for (unsigned int maskBit = 0; maskBit < 32; maskBit++)
			m_ExtraMasks[maskBit] = GetBitForBitnum(int(maskBit)) - 1;
		m_ExtraMasks[32] = ~0ul;

		for (unsigned int littleBit = 0; littleBit < 32; littleBit++)
			StoreLittleDWord((unsigned int*)&m_LittleBits[littleBit], 0, 1u << littleBit);
	}

	// Precalculated bit masks for WriteUBitLong. Using these tables instead of 
	// doing the calculations gives a 33% speedup in WriteUBitLong.
	unsigned long m_BitWriteMasks[32][33];
	unsigned long m_LittleBits[32];
	unsigned long m_ExtraMasks[33]; // (1 << i) - 1
};
static CBitWriteMasks s_BitWriteMasks;


//-----------------------------------------------------------------------------
// Error handler
//-----------------------------------------------------------------------------
static BitBufErrorHandler g_BitBufErrorHandler = 0;
void InternalBitBufErrorHandler(BitBufErrorType errorType, const char* pDebugName)
{
	if (g_BitBufErrorHandler)
		g_BitBufErrorHandler(errorType, pDebugName);
}
void SetBitBufErrorHandler(BitBufErrorHandler fn)
{
	g_BitBufErrorHandler = fn;
}

// ---------------------------------------------------------------------------------------- //
// CBitBuffer
// ---------------------------------------------------------------------------------------- //

CBitBuffer::CBitBuffer(void)
{
	m_bOverflow = false;
	m_pDebugName = NULL;
	m_nDataBits = -1;
	m_nDataBytes = 0;
}

// ---------------------------------------------------------------------------------------- //
// bf_read
// ---------------------------------------------------------------------------------------- //
CBitRead::CBitRead(const void* pData, int nBytes, int nBits /*= -1*/)
{
	StartReading(pData, nBytes, 0, nBits);
}

CBitRead::CBitRead(const char* pDebugName, const void* pData, int nBytes, int nBits /*= -1*/)
{
	SetDebugName(pDebugName);
	StartReading(pData, nBytes, 0, nBits);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBitRead::StartReading(const void* pData, size_t nBytes, int64 iStartBit, int64 nBits)
{
	// Make sure it's dword aligned and padded.
	assert((int64(pData) & 3) == 0);
	m_pData = (uint32*)pData;
	m_pDataIn = m_pData;
	m_nDataBytes = nBytes;

	if (nBits == -1)
	{
		m_nDataBits = nBytes << 3;
	}
	else
	{
		assert(nBits <= int64(nBytes * 8));
		m_nDataBits = nBits;
	}
	m_bOverflow = false;
	m_pBufferEnd = reinterpret_cast<uint32 const*> (reinterpret_cast<uint8 const*> (m_pData) + nBytes);
	if (m_pData)
		Seek(iStartBit);

}

//-----------------------------------------------------------------------------
// Purpose: seeks to a specific position in the buffer
//-----------------------------------------------------------------------------
bool CBitRead::Seek(int64 nPosition)
{
	bool bSucc = true;
	if (nPosition < 0 || nPosition > m_nDataBits)
	{
		SetOverflowFlag();
		bSucc = false;
		nPosition = m_nDataBits;
	}
	size_t nHead = m_nDataBytes & 3;						// non-multiple-of-4 bytes at head of buffer. We put the "round off"
	// at the head to make reading and detecting the end efficient.

	size_t nByteOfs = nPosition / 8;
	if ((m_nDataBytes < 4) || (nHead && (nByteOfs < nHead)))
	{
		// partial first dword
		uint8 const* pPartial = (uint8 const*)m_pData;
		if (m_pData)
		{
			m_nInBufWord = *(pPartial++);
			if (nHead > 1)
				m_nInBufWord |= (*pPartial++) << 8;
			if (nHead > 2)
				m_nInBufWord |= (*pPartial++) << 16;
		}
		m_pDataIn = (uint32 const*)pPartial;
		m_nInBufWord >>= (nPosition & 31);
		m_nBitsAvail = int((nHead << 3) - (nPosition & 31));
	}
	else
	{
		ssize_t nAdjPosition = nPosition - (nHead << 3);
		m_pDataIn = reinterpret_cast<uint32 const*> (
			reinterpret_cast<uint8 const*>(m_pData) + ((nAdjPosition / 32) << 2) + nHead);
		if (m_pData)
		{
			m_nBitsAvail = 32;
			GrabNextDWord();
		}
		else
		{
			m_nInBufWord = 0;
			m_nBitsAvail = 1;
		}
		m_nInBufWord >>= (nAdjPosition & 31);
		m_nBitsAvail = min(m_nBitsAvail, 32 - (nAdjPosition & 31));	// in case grabnextdword overflowed
	}
	return bSucc;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBitRead::GrabNextDWord(bool bOverFlowImmediately)
{
	if (m_pDataIn == m_pBufferEnd)
	{
		m_nBitsAvail = 1;
		m_nInBufWord = 0;

		m_pDataIn++;

		if (bOverFlowImmediately)
			SetOverflowFlag();
	}
	else
	{
		if (m_pDataIn > m_pBufferEnd)
		{
			SetOverflowFlag();
			m_nInBufWord = 0;
		}
		else
		{
			assert(reinterpret_cast<uintptr_t>(m_pDataIn) + 3 < reinterpret_cast<uintptr_t>(m_pBufferEnd));
			m_nInBufWord = LittleDWord(*(m_pDataIn++));
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBitRead::FetchNext()
{
	m_nBitsAvail = 32;
	GrabNextDWord(false);
}

//-----------------------------------------------------------------------------
// Purpose: reads an unsigned integer from the buffer
//-----------------------------------------------------------------------------
uint32 CBitRead::ReadUBitLong(int numbits)
{
	if (m_nBitsAvail >= numbits)
	{
		unsigned int nRet = m_nInBufWord & s_BitWriteMasks.m_ExtraMasks[numbits];
		m_nBitsAvail -= numbits;
		if (m_nBitsAvail)
		{
			m_nInBufWord >>= numbits;
		}
		else
		{
			FetchNext();
		}
		return nRet;
	}
	else
	{
		uint32 nRet = m_nInBufWord;
		numbits -= m_nBitsAvail;
		GrabNextDWord(true);

		if (IsOverflowed())
			return 0;

		nRet |= ((m_nInBufWord & s_BitWriteMasks.m_ExtraMasks[numbits]) << m_nBitsAvail);
		m_nBitsAvail = 32 - numbits;
		m_nInBufWord >>= numbits;

		return nRet;
	}
}

//-----------------------------------------------------------------------------
// Purpose: reads a signed integer from the buffer
//-----------------------------------------------------------------------------
int CBitRead::ReadSBitLong(int numbits)
{
	int nRet = ReadUBitLong(numbits);
	return (nRet << (32 - numbits)) >> (32 - numbits);
}

//-----------------------------------------------------------------------------
// Purpose: reads a signed 64-bit integer from the buffer
//-----------------------------------------------------------------------------
int64 CBitRead::ReadLongLong()
{
	int64 retval;
	uint* pLongs = (uint*)&retval;

	// Read the two DWORDs according to network endian
	const short endianIndex = 0x0100;
	byte* idx = (byte*)&endianIndex;
	pLongs[*idx++] = ReadUBitLong(sizeof(long) << 3);
	pLongs[*idx] = ReadUBitLong(sizeof(long) << 3);

	return retval;
}

//-----------------------------------------------------------------------------
// Purpose: reads a float from the buffer
//-----------------------------------------------------------------------------
float CBitRead::ReadFloat()
{
	float ret;
	Assert(sizeof(ret) == 4);
	ReadBits(&ret, 32);

	// Swap the float, since ReadBits reads raw data
	LittleFloat(&ret, &ret);
	return ret;
}

//-----------------------------------------------------------------------------
// Purpose: reads bits from the buffer
//-----------------------------------------------------------------------------
void CBitRead::ReadBits(void* pOutData, int nBits)
{
	unsigned char* pOut = (unsigned char*)pOutData;
	int nBitsLeft = nBits;

	// align output to dword boundary
	while (((uintp)pOut & 3) != 0 && nBitsLeft >= 8)
	{
		*pOut = (unsigned char)ReadUBitLong(8);
		++pOut;
		nBitsLeft -= 8;
	}

	// X360TBD: Can't read dwords in ReadBits because they'll get swapped
	if (IsPC())
	{
		// read dwords
		while (nBitsLeft >= 32)
		{
			*((uint32*)pOut) = ReadUBitLong(32);
			pOut += sizeof(uint32);
			nBitsLeft -= 32;
		}
	}

	// read remaining bytes
	while (nBitsLeft >= 8)
	{
		*pOut = (unsigned char)ReadUBitLong(8);
		++pOut;
		nBitsLeft -= 8;
	}

	// read remaining bits
	if (nBitsLeft)
	{
		*pOut = (unsigned char)ReadUBitLong(nBitsLeft);
	}
}

//-----------------------------------------------------------------------------
// Purpose: reads bytes from the buffer
//-----------------------------------------------------------------------------
bool CBitRead::ReadBytes(void* pOut, int nBytes)
{
	ReadBits(pOut, nBytes << 3);
	return !IsOverflowed();
}

//-----------------------------------------------------------------------------
// Purpose: reads a string from the buffer
//-----------------------------------------------------------------------------
bool CBitRead::ReadString(char* pStr, int maxLen, bool bLine, int* pOutNumChars)
{
	assert(maxLen != 0);

	bool bTooSmall = false;
	int iChar = 0;
	while (1)
	{
		char val = char(ReadChar());
		if (val == 0)
			break;
		else if (bLine && val == '\n')
			break;

		if (iChar < (maxLen - 1))
		{
			pStr[iChar] = val;
			++iChar;
		}
		else
		{
			bTooSmall = true;
		}
	}

	// Make sure it's null-terminated.
	pStr[iChar] = '\0';

	if (pOutNumChars)
		*pOutNumChars = iChar;

	return !IsOverflowed() && !bTooSmall;
}

// ---------------------------------------------------------------------------------------- //
// bf_write
// ---------------------------------------------------------------------------------------- //
CBitWrite::CBitWrite()
{
	//DEBUG_LINK_CHECK;
	m_pData = NULL;
	m_nDataBytes = 0;
	m_nDataBits = -1; // set to -1 so we generate overflow on any operation
	m_iCurBit = 0;
	m_bOverflow = false;
	m_bAssertOnOverflow = true;
	m_pDebugName = NULL;
}
CBitWrite::CBitWrite(const char* pDebugName, void* pData, int nBytes, int nBits)
{
	//DEBUG_LINK_CHECK;
	m_bAssertOnOverflow = true;
	m_pDebugName = pDebugName;
	StartWriting(pData, nBytes, 0, nBits);
}
CBitWrite::CBitWrite(void* pData, int nBytes, int nBits)
{
	m_bAssertOnOverflow = true;
	m_pDebugName = NULL;
	StartWriting(pData, nBytes, 0, nBits);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBitWrite::StartWriting(void* pData, int nBytes, int iStartBit, int nBits)
{
	// Make sure it's dword aligned and padded.
	//DEBUG_LINK_CHECK;
	Assert((nBytes % 4) == 0);
	Assert(((uintp)pData & 3) == 0);

	// The writing code will overrun the end of the buffer if it isn't dword aligned, so truncate to force alignment
	nBytes &= ~3;

	m_pData = (unsigned char*)pData;
	m_nDataBytes = nBytes;

	if (nBits == -1)
	{
		m_nDataBits = nBytes << 3;
	}
	else
	{
		Assert(nBits <= nBytes * 8);
		m_nDataBits = nBits;
	}

	m_iCurBit = iStartBit;
	m_bOverflow = false;
}

//-----------------------------------------------------------------------------
// Purpose: writes a bit to the buffer without checking for overflow
//-----------------------------------------------------------------------------
inline void CBitWrite::WriteOneBitNoCheck(int nValue)
{
	if (nValue)
		m_pData[m_iCurBit >> 3] |= (1 << (m_iCurBit & 7));
	else
		m_pData[m_iCurBit >> 3] &= ~(1 << (m_iCurBit & 7));

	++m_iCurBit;
}

//-----------------------------------------------------------------------------
// Purpose: writes a bit to the buffer
//-----------------------------------------------------------------------------
inline void CBitWrite::WriteOneBit(int nValue)
{
	if (!CheckForOverflow(1))
		WriteOneBitNoCheck(nValue);
}

//-----------------------------------------------------------------------------
// Purpose: writes a bit to the buffer at a specific bit index
//-----------------------------------------------------------------------------
inline void	CBitWrite::WriteOneBitAt(int iBit, int nValue)
{
	if (iBit + 1 > m_nDataBits)
	{
		SetOverflowFlag();
		CallErrorHandler(BITBUFERROR_BUFFER_OVERRUN, GetDebugName());
		return;
	}

	if (nValue)
		m_pData[iBit >> 3] |= (1 << (iBit & 7));
	else
		m_pData[iBit >> 3] &= ~(1 << (iBit & 7));
}

//-----------------------------------------------------------------------------
// Purpose: writes an unsigned integer to the buffer
//-----------------------------------------------------------------------------
/*BITBUF_INLINE*/ void CBitWrite::WriteUBitLong(unsigned int curData, int numbits, bool bCheckRange)
{
#ifdef _DEBUG
	// Make sure it doesn't overflow.
	if (bCheckRange && numbits < 32)
	{
		if (curData >= (uint32)(1 << numbits))
		{
			CallErrorHandler(BITBUFERROR_VALUE_OUT_OF_RANGE, GetDebugName());
		}
	}
	Assert(numbits >= 0 && numbits <= 32);
#else
	NOTE_UNUSED(bCheckRange);
#endif

	// Bounds checking..
	if ((m_iCurBit + numbits) > m_nDataBits)
	{
		m_iCurBit = m_nDataBits;
		SetOverflowFlag();
		CallErrorHandler(BITBUFERROR_BUFFER_OVERRUN, GetDebugName());
		return;
	}

	int nBitsLeft = numbits;
	int iCurBit = m_iCurBit;

	// Mask in a dword.
	unsigned int iDWord = iCurBit >> 5;
	Assert((iDWord * 4 + sizeof(int32)) <= (unsigned int)m_nDataBytes);

	uint32 iCurBitMasked = iCurBit & 31;

	uint32 dword = LoadLittleDWord((uint32*)m_pData, iDWord);

	dword &= s_BitWriteMasks.m_BitWriteMasks[iCurBitMasked][nBitsLeft];
	dword |= curData << iCurBitMasked;

	// write to stream (lsb to msb) properly
	StoreLittleDWord((uint32*)m_pData, iDWord, dword);

	// Did it span a dword?
	int nBitsWritten = 32 - iCurBitMasked;
	if (nBitsWritten < nBitsLeft)
	{
		nBitsLeft -= nBitsWritten;
		curData >>= nBitsWritten;

		// read from stream (lsb to msb) properly 
		dword = LoadLittleDWord((uint32*)m_pData, iDWord + 1);

		dword &= s_BitWriteMasks.m_BitWriteMasks[0][nBitsLeft];
		dword |= curData;

		// write to stream (lsb to msb) properly 
		StoreLittleDWord((uint32*)m_pData, iDWord + 1, dword);
	}

	m_iCurBit += numbits;
}

//-----------------------------------------------------------------------------
// Purpose: writes a signed integer to the buffer
// (Sign bit comes first)
//-----------------------------------------------------------------------------
void CBitWrite::WriteSBitLong(int data, int numbits)
{
	// Do we have a valid # of bits to encode with?
	Assert(numbits >= 1);

	// Note: it does this weirdness here so it's bit-compatible with regular integer data in the buffer.
	// (Some old code writes direct integers right into the buffer).
	if (data < 0)
	{
#ifdef _DEBUG
		if (numbits < 32)
		{
			// Make sure it doesn't overflow.

			if (data < 0)
			{
				Assert(data >= -(GetBitForBitnum(numbits - 1)));
			}
			else
			{
				Assert(data < (GetBitForBitnum(numbits - 1)));
			}
		}
#endif

		WriteUBitLong((unsigned int)(0x80000000 + data), numbits - 1, false);
		WriteOneBit(1);
	}
	else
	{
		WriteUBitLong((unsigned int)data, numbits - 1);
		WriteOneBit(0);
	}
}
//-----------------------------------------------------------------------------
// Purpose: writes a signed or unsigned integer to the buffer
//-----------------------------------------------------------------------------
void CBitWrite::WriteBitLong(unsigned int data, int numbits, bool bSigned)
{
	if (bSigned)
		WriteSBitLong((int)data, numbits);
	else
		WriteUBitLong(data, numbits);
}

//-----------------------------------------------------------------------------
// Purpose: writes a list of bits to the buffer
//-----------------------------------------------------------------------------
bool CBitWrite::WriteBits(const void* pInData, int nBits)
{
	unsigned char* pIn = (unsigned char*)pInData;
	int nBitsLeft = nBits;

	// Bounds checking..
	if ((m_iCurBit + nBits) > m_nDataBits)
	{
		SetOverflowFlag();
		CallErrorHandler(BITBUFERROR_BUFFER_OVERRUN, GetDebugName());
		return false;
	}

	// Align input to dword boundary
	while (((uintp)pIn & 3) != 0 && nBitsLeft >= 8)
	{
		WriteUBitLong(*pIn, 8, false);
		++pIn;
		nBitsLeft -= 8;
	}

	if (nBitsLeft >= 32)
	{
		if ((m_iCurBit & 7) == 0)
		{
			// current bit is byte aligned, do block copy
			int numbytes = nBitsLeft >> 3;
			int numbits = numbytes << 3;

			memcpy(m_pData + (m_iCurBit >> 3), pIn, numbytes);
			pIn += numbytes;
			nBitsLeft -= numbits;
			m_iCurBit += numbits;
		}
		else
		{
			const uint32 iBitsRight = (m_iCurBit & 31);
			Assert(iBitsRight > 0); // should not be aligned, otherwise it would have been handled before
			const uint32 iBitsLeft = 32 - iBitsRight;
			const int iBitsChanging = 32 + iBitsLeft; // how many bits are changed during one step (not necessary written meaningful)
			unsigned int iDWord = m_iCurBit >> 5;

			uint32 outWord = LoadLittleDWord((uint32*)m_pData, iDWord);
			outWord &= s_BitWriteMasks.m_BitWriteMasks[iBitsRight][32]; // clear rest of beginning DWORD 

			// copy in DWORD blocks
			while (nBitsLeft >= iBitsChanging)
			{
				uint32 curData = LittleDWord(*(uint32*)pIn);
				pIn += sizeof(uint32);

				outWord |= curData << iBitsRight;
				StoreLittleDWord((uint32*)m_pData, iDWord, outWord);

				++iDWord;
				outWord = curData >> iBitsLeft;

				nBitsLeft -= 32;
				m_iCurBit += 32;
			}

			// store last word
			StoreLittleDWord((uint32*)m_pData, iDWord, outWord);

			// write remaining DWORD 
			if (nBitsLeft >= 32)
			{
				WriteUBitLong(LittleDWord(*((uint32*)pIn)), 32, false);
				pIn += sizeof(uint32);
				nBitsLeft -= 32;
			}
		}
	}

	// write remaining bytes
	while (nBitsLeft >= 8)
	{
		WriteUBitLong(*pIn, 8, false);
		++pIn;
		nBitsLeft -= 8;
	}

	// write remaining bits
	if (nBitsLeft)
	{
		WriteUBitLong(*pIn, nBitsLeft, false);
	}

	return !IsOverflowed();
}

//-----------------------------------------------------------------------------
// Purpose: checks if we have enough space for the requested number of bits
//-----------------------------------------------------------------------------
bool CBitWrite::CheckForOverflow(int nBits)
{
	if (this->m_iCurBit + nBits > this->m_nDataBits)
	{
		this->SetOverflowFlag();
	}

	return IsOverflowed();
}

//-----------------------------------------------------------------------------
// Purpose: sets the overflow flag
//-----------------------------------------------------------------------------
void CBitWrite::SetOverflowFlag()
{
	if (this->m_bAssertOnOverflow)
	{
		assert(false);
	}

	this->m_bOverflow = true;
}
