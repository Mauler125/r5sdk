//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: buffer serialization/deserialization.
//
// $NoKeywords: $
//===========================================================================//

#include "tier1/bitbuf.h"
#include "mathlib/bitvec.h"
#include "public/coordsize.h"

// Precalculated bit masks for WriteUBitLong. Using these tables instead of 
// doing the calculations gives a 33% speedup in WriteUBitLong.
uint32 g_BitWriteMasks[32][33];

// (1 << i) - 1
uint32 g_ExtraMasks[32];

//-----------------------------------------------------------------------------
// Read/Write masks
//-----------------------------------------------------------------------------
class CBitWriteMasksInit
{
public:
	CBitWriteMasksInit()
	{
		for (unsigned int startbit = 0; startbit < 32; startbit++)
		{
			for (unsigned int nBitsLeft = 0; nBitsLeft < 33; nBitsLeft++)
			{
				unsigned int endbit = startbit + nBitsLeft;
				g_BitWriteMasks[startbit][nBitsLeft] = GetBitForBitnum(startbit) - 1;
				if (endbit < 32)
					g_BitWriteMasks[startbit][nBitsLeft] |= ~(GetBitForBitnum(endbit) - 1);
			}
		}

		for (unsigned int maskBit = 0; maskBit < 32; maskBit++)
			g_ExtraMasks[maskBit] = GetBitForBitnum(maskBit) - 1;
	}
};
static CBitWriteMasksInit s_BitWriteMasksInit;

const uint32 CBitBuffer::s_nMaskTable[33] = {
	0,
	(1 << 1) - 1,
	(1 << 2) - 1,
	(1 << 3) - 1,
	(1 << 4) - 1,
	(1 << 5) - 1,
	(1 << 6) - 1,
	(1 << 7) - 1,
	(1 << 8) - 1,
	(1 << 9) - 1,
	(1 << 10) - 1,
	(1 << 11) - 1,
	(1 << 12) - 1,
	(1 << 13) - 1,
	(1 << 14) - 1,
	(1 << 15) - 1,
	(1 << 16) - 1,
	(1 << 17) - 1,
	(1 << 18) - 1,
	(1 << 19) - 1,
	(1 << 20) - 1,
	(1 << 21) - 1,
	(1 << 22) - 1,
	(1 << 23) - 1,
	(1 << 24) - 1,
	(1 << 25) - 1,
	(1 << 26) - 1,
	(1 << 27) - 1,
	(1 << 28) - 1,
	(1 << 29) - 1,
	(1 << 30) - 1,
	0x7fffffff,
	0xffffffff,
};

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
CBitRead::CBitRead(const void* pData, size_t nBytes, ssize_t nBits /*= -1*/)
{
	StartReading(pData, nBytes, 0, nBits);
}
CBitRead::CBitRead(const char* pDebugName, const void* pData, size_t nBytes, ssize_t nBits /*= -1*/)
{
	SetDebugName(pDebugName);
	StartReading(pData, nBytes, 0, nBits);
}


///////////////////////////////////////////////////////////////////////////////
// Core bf_read routines
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
void CBitRead::StartReading(const void* pData, size_t nBytes, ssize_t iStartBit, ssize_t nBits)
{
	// Make sure it's dword aligned and padded.
	assert((ssize_t(pData) & 3) == 0);

	m_pData = (uint32*)pData;
	m_pDataIn = m_pData;
	m_nDataBytes = nBytes;

	if (nBits == -1)
	{
		m_nDataBits = nBytes << 3;
	}
	else
	{
		assert(nBits <= (ssize_t)(nBytes * 8));
		m_nDataBits = nBits;
	}

	m_bOverflow = false;
	m_pBufferEnd = reinterpret_cast<uint32 const*> (reinterpret_cast<uint8 const*> (m_pData) + nBytes);

	if (m_pData)
		Seek(iStartBit);

}

//-----------------------------------------------------------------------------
// seeks to a specific position in the buffer
bool CBitRead::Seek(ssize_t nPosition)
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
		m_nBitsAvail = MIN(m_nBitsAvail, 32 - (nAdjPosition & 31));	// in case grabnextdword overflowed
	}
	return bSucc;
}


///////////////////////////////////////////////////////////////////////////////
// Routines for reading coordinates from the buffer (these contain bit-field
// size AND fixed point scaling constants)
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
float CBitRead::ReadBitCoord(void)
{
	int		intval = 0, fractval = 0, signbit = 0;
	float	value = 0.0;


	// Read the required integer and fraction flags
	intval = ReadOneBit();
	fractval = ReadOneBit();

	// If we got either parse them, otherwise it's a zero.
	if (intval || fractval)
	{
		// Read the sign bit
		signbit = ReadOneBit();

		// If there's an integer, read it in
		if (intval)
		{
			// Adjust the integers from [0..MAX_COORD_VALUE-1] to [1..MAX_COORD_VALUE]
			intval = ReadUBitLong(COORD_INTEGER_BITS) + 1;
		}

		// If there's a fraction, read it in
		if (fractval)
		{
			fractval = ReadUBitLong(COORD_FRACTIONAL_BITS);
		}

		// Calculate the correct floating point value
		value = intval + (float)((float)fractval * COORD_RESOLUTION);

		// Fixup the sign if negative.
		if (signbit)
			value = -value;
	}

	return value;
}

//-----------------------------------------------------------------------------
float CBitRead::ReadBitCoordMP(EBitCoordType coordType)
{
	bool bIntegral = (coordType == kCW_Integral);
	bool bLowPrecision = (coordType == kCW_LowPrecision);

	int		intval = 0, fractval = 0, signbit = 0;
	float	value = 0.0;

	bool bInBounds = ReadOneBit() ? true : false;

	if (bIntegral)
	{
		// Read the required integer and fraction flags
		intval = ReadOneBit();
		// If we got either parse them, otherwise it's a zero.
		if (intval)
		{
			// Read the sign bit
			signbit = ReadOneBit();

			// If there's an integer, read it in
			// Adjust the integers from [0..MAX_COORD_VALUE-1] to [1..MAX_COORD_VALUE]
			if (bInBounds)
			{
				value = (float)ReadUBitLong(COORD_INTEGER_BITS_MP) + 1;
			}
			else
			{
				value = (float)ReadUBitLong(COORD_INTEGER_BITS) + 1;
			}
		}
	}
	else
	{
		// Read the required integer and fraction flags
		intval = ReadOneBit();

		// Read the sign bit
		signbit = ReadOneBit();

		// If we got either parse them, otherwise it's a zero.
		if (intval)
		{
			if (bInBounds)
			{
				intval = ReadUBitLong(COORD_INTEGER_BITS_MP) + 1;
			}
			else
			{
				intval = ReadUBitLong(COORD_INTEGER_BITS) + 1;
			}
		}

		// If there's a fraction, read it in
		fractval = ReadUBitLong(bLowPrecision ? COORD_FRACTIONAL_BITS_MP_LOWPRECISION : COORD_FRACTIONAL_BITS);

		// Calculate the correct floating point value
		value = intval + ((float)fractval * (bLowPrecision ? COORD_RESOLUTION_LOWPRECISION : COORD_RESOLUTION));
	}

	// Fixup the sign if negative.
	if (signbit)
		value = -value;

	return value;
}

//-----------------------------------------------------------------------------
float CBitRead::ReadBitCellCoord(int bits, EBitCoordType coordType)
{
#if defined( BB_PROFILING )
	VPROF("CBitRead::ReadBitCoordMP");
#endif
	bool bIntegral = (coordType == kCW_Integral);
	bool bLowPrecision = (coordType == kCW_LowPrecision);

	int		intval = 0, fractval = 0;
	float	value = 0.0;

	if (bIntegral)
	{
		value = (float)ReadUBitLong(bits);
	}
	else
	{
		intval = ReadUBitLong(bits);

		// If there's a fraction, read it in
		fractval = ReadUBitLong(bLowPrecision ? COORD_FRACTIONAL_BITS_MP_LOWPRECISION : COORD_FRACTIONAL_BITS);

		// Calculate the correct floating point value
		value = intval + ((float)fractval * (bLowPrecision ? COORD_RESOLUTION_LOWPRECISION : COORD_RESOLUTION));
	}

	return value;
}

//-----------------------------------------------------------------------------
float CBitRead::ReadBitNormal(void)
{
	// Read the sign bit
	int	signbit = ReadOneBit();

	// Read the fractional part
	unsigned int fractval = ReadUBitLong(NORMAL_FRACTIONAL_BITS);

	// Calculate the correct floating point value
	float value = (float)fractval * NORMAL_RESOLUTION;

	// Fixup the sign if negative.
	if (signbit)
		value = -value;

	return value;
}

//-----------------------------------------------------------------------------
void CBitRead::ReadBitVec3Coord(Vector3D& fa)
{
	int		xflag, yflag, zflag;

	// This vector must be initialized! Otherwise, If any of the flags aren't set, 
	// the corresponding component will not be read and will be stack garbage.
	fa.Init(0, 0, 0);

	xflag = ReadOneBit();
	yflag = ReadOneBit();
	zflag = ReadOneBit();

	if (xflag)
		fa[0] = ReadBitCoord();
	if (yflag)
		fa[1] = ReadBitCoord();
	if (zflag)
		fa[2] = ReadBitCoord();
}

//-----------------------------------------------------------------------------
void CBitRead::ReadBitVec3Normal(Vector3D& fa)
{
	int xflag = ReadOneBit();
	int yflag = ReadOneBit();

	if (xflag)
		fa[0] = ReadBitNormal();
	else
		fa[0] = 0.0f;

	if (yflag)
		fa[1] = ReadBitNormal();
	else
		fa[1] = 0.0f;

	// The first two imply the third (but not its sign)
	int znegative = ReadOneBit();

	float fafafbfb = fa[0] * fa[0] + fa[1] * fa[1];
	if (fafafbfb < 1.0f)
		fa[2] = sqrt(1.0f - fafafbfb);
	else
		fa[2] = 0.0f;

	if (znegative)
		fa[2] = -fa[2];
}

//-----------------------------------------------------------------------------
void CBitRead::ReadBitAngles(QAngle& fa)
{
	Vector3D tmp;
	ReadBitVec3Coord(tmp);
	fa.Init(tmp.x, tmp.y, tmp.z);
}

//-----------------------------------------------------------------------------
float CBitRead::ReadBitAngle(int numbits)
{
	float shift = (float)(GetBitForBitnum(numbits));

	int i = ReadUBitLong(numbits);
	float fReturn = (float)i * (360.0f / shift);

	return fReturn;
}


///////////////////////////////////////////////////////////////////////////////
// Routines for reading encoded (var)ints from the buffer
///////////////////////////////////////////////////////////////////////////////

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
// Read 1-5 bytes in order to extract a 32-bit unsigned value from the
// stream. 7 data bits are extracted from each byte with the 8th bit used
// to indicate whether the loop should continue.
// This allows variable size numbers to be stored with tolerable
// efficiency. Numbers sizes that can be stored for various numbers of
// encoded bits are:
//  8-bits: 0-127
// 16-bits: 128-16383
// 24-bits: 16384-2097151
// 32-bits: 2097152-268435455
// 40-bits: 268435456-0xFFFFFFFF
uint32 CBitRead::ReadVarInt32()
{
	uint32 result = 0;
	int count = 0;
	uint32 b;

	do
	{
		if (count == bitbuf::kMaxVarint32Bytes)
		{
			return result;
		}
		b = ReadUBitLong(8);
		result |= (b & 0x7F) << (7 * count);
		++count;
	} while (b & 0x80);

	return result;
}

//-----------------------------------------------------------------------------
uint64 CBitRead::ReadVarInt64()
{
	uint64 result = 0;
	int count = 0;
	uint64 b;

	do
	{
		if (count == bitbuf::kMaxVarint64Bytes)
		{
			return result;
		}
		b = ReadUBitLong(8);
		result |= static_cast<uint64>(b & 0x7F) << (7 * count);
		++count;
	} while (b & 0x80);

	return result;
}


///////////////////////////////////////////////////////////////////////////////
// Routines for reading bits and bytes from the buffer
///////////////////////////////////////////////////////////////////////////////

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
bool CBitRead::ReadBytes(void* pOut, int nBytes)
{
	ReadBits(pOut, nBytes << 3);
	return !IsOverflowed();
}


///////////////////////////////////////////////////////////////////////////////
// Routines for reading encoded strings from the buffer
///////////////////////////////////////////////////////////////////////////////

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

//-----------------------------------------------------------------------------
bool CBitRead::ReadWString(OUT_Z_CAP(maxLenInChars) wchar_t* pStr, int maxLenInChars, bool bLine, int* pOutNumChars)
{
	Assert(maxLenInChars != 0);

	bool bTooSmall = false;
	int iChar = 0;
	while (1)
	{
		wchar val = (wchar)ReadShort();
		if (val == 0)
			break;
		else if (bLine && val == L'\n')
			break;

		if (iChar < (maxLenInChars - 1))
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
	Assert(iChar < maxLenInChars);
	pStr[iChar] = 0;

	if (pOutNumChars)
		*pOutNumChars = iChar;

	return !IsOverflowed() && !bTooSmall;
}

//-----------------------------------------------------------------------------
char* CBitRead::ReadAndAllocateString(bool* pOverflow)
{
	char str[2048];

	int nChars;
	bool bOverflow = !ReadString(str, sizeof(str), false, &nChars);
	if (pOverflow)
		*pOverflow = bOverflow;

	// Now copy into the output and return it;
	char* pRet = new char[nChars + 1];
	for (int i = 0; i <= nChars; i++)
		pRet[i] = str[i];

	return pRet;
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


///////////////////////////////////////////////////////////////////////////////
// Core bf_write routines
///////////////////////////////////////////////////////////////////////////////

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
// checks if we have enough space for the requested number of bits
bool CBitWrite::CheckForOverflow(int nBits)
{
	if (this->m_iCurBit + nBits > this->m_nDataBits)
	{
		this->SetOverflowFlag();
	}

	return IsOverflowed();
}

//-----------------------------------------------------------------------------
// sets the overflow flag
void CBitWrite::SetOverflowFlag()
{
	if (this->m_bAssertOnOverflow)
	{
		assert(false);
	}

	this->m_bOverflow = true;
}


///////////////////////////////////////////////////////////////////////////////
// Routines for writing integers into the buffer
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// writes an unsigned integer into the buffer
void CBitWrite::WriteUBitLong(unsigned int curData, int numbits, bool bCheckRange)
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

	dword &= g_BitWriteMasks[iCurBitMasked][nBitsLeft];
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

		dword &= g_BitWriteMasks[0][nBitsLeft];
		dword |= curData;

		// write to stream (lsb to msb) properly 
		StoreLittleDWord((uint32*)m_pData, iDWord + 1, dword);
	}

	m_iCurBit += numbits;
}

//-----------------------------------------------------------------------------
// NOTE: Sign bit comes first
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
void CBitWrite::WriteBitLong(unsigned int data, int numbits, bool bSigned)
{
	if (bSigned)
		WriteSBitLong((int)data, numbits);
	else
		WriteUBitLong(data, numbits);
}

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
			outWord &= g_BitWriteMasks[iBitsRight][32]; // clear rest of beginning DWORD 

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
bool CBitWrite::WriteBitsFromBuffer(bf_read* pIn, int nBits)
{
	// This could be optimized a little by
	while (nBits > 32)
	{
		WriteUBitLong(pIn->ReadUBitLong(32), 32);
		nBits -= 32;
	}

	WriteUBitLong(pIn->ReadUBitLong(nBits), nBits);
	return !IsOverflowed() && !pIn->IsOverflowed();
}


///////////////////////////////////////////////////////////////////////////////
// Routines for writing integers with variable bit length into the buffer
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
void CBitWrite::WriteVarInt32(uint32 data)
{
	// Check if align and we have room, slow path if not
	if ((m_iCurBit & 7) == 0 && (m_iCurBit + bitbuf::kMaxVarint32Bytes * 8) <= m_nDataBits)
	{
		uint8* target = ((uint8*)m_pData) + (m_iCurBit >> 3);

		target[0] = static_cast<uint8>(data | 0x80);
		if (data >= (1 << 7))
		{
			target[1] = static_cast<uint8>((data >> 7) | 0x80);
			if (data >= (1 << 14))
			{
				target[2] = static_cast<uint8>((data >> 14) | 0x80);
				if (data >= (1 << 21))
				{
					target[3] = static_cast<uint8>((data >> 21) | 0x80);
					if (data >= (1 << 28))
					{
						target[4] = static_cast<uint8>(data >> 28);
						m_iCurBit += 5 * 8;
						return;
					}
					else
					{
						target[3] &= 0x7F;
						m_iCurBit += 4 * 8;
						return;
					}
				}
				else
				{
					target[2] &= 0x7F;
					m_iCurBit += 3 * 8;
					return;
				}
			}
			else
			{
				target[1] &= 0x7F;
				m_iCurBit += 2 * 8;
				return;
			}
		}
		else
		{
			target[0] &= 0x7F;
			m_iCurBit += 1 * 8;
			return;
		}
	}
	else // Slow path
	{
		while (data > 0x7F)
		{
			WriteUBitLong((data & 0x7F) | 0x80, 8);
			data >>= 7;
		}
		WriteUBitLong(data & 0x7F, 8);
	}
}

//-----------------------------------------------------------------------------
void CBitWrite::WriteVarInt64(uint64 data)
{
	// Check if align and we have room, slow path if not
	if ((m_iCurBit & 7) == 0 && (m_iCurBit + bitbuf::kMaxVarint64Bytes * 8) <= m_nDataBits)
	{
		uint8* target = ((uint8*)m_pData) + (m_iCurBit >> 3);

		// Splitting into 32-bit pieces gives better performance on 32-bit
		// processors.
		uint32 part0 = static_cast<uint32>(data);
		uint32 part1 = static_cast<uint32>(data >> 28);
		uint32 part2 = static_cast<uint32>(data >> 56);

		int size;

		// Here we can't really optimize for small numbers, since the data is
		// split into three parts.  Cheking for numbers < 128, for instance,
		// would require three comparisons, since you'd have to make sure part1
		// and part2 are zero.  However, if the caller is using 64-bit integers,
		// it is likely that they expect the numbers to often be very large, so
		// we probably don't want to optimize for small numbers anyway.  Thus,
		// we end up with a hardcoded binary search tree...
		if (part2 == 0)
		{
			if (part1 == 0)
			{
				if (part0 < (1 << 14))
				{
					if (part0 < (1 << 7))
					{
						size = 1; goto size1;
					}
					else
					{
						size = 2; goto size2;
					}
				}
				else
				{
					if (part0 < (1 << 21))
					{
						size = 3; goto size3;
					}
					else
					{
						size = 4; goto size4;
					}
				}
			}
			else
			{
				if (part1 < (1 << 14))
				{
					if (part1 < (1 << 7))
					{
						size = 5; goto size5;
					}
					else
					{
						size = 6; goto size6;
					}
				}
				else
				{
					if (part1 < (1 << 21))
					{
						size = 7; goto size7;
					}
					else
					{
						size = 8; goto size8;
					}
				}
			}
		}
		else
		{
			if (part2 < (1 << 7))
			{
				size = 9; goto size9;
			}
			else
			{
				size = 10; goto size10;
			}
		}

		// commented as this would otherwise trigger a compiled warning in MSVC
		// which confirms this code is unreachable
		//AssertFatalMsg(false, "Can't get here.");

	size10: target[9] = static_cast<uint8>((part2 >> 7) | 0x80);
	size9: target[8] = static_cast<uint8>((part2) | 0x80);
	size8: target[7] = static_cast<uint8>((part1 >> 21) | 0x80);
	size7: target[6] = static_cast<uint8>((part1 >> 14) | 0x80);
	size6: target[5] = static_cast<uint8>((part1 >> 7) | 0x80);
	size5: target[4] = static_cast<uint8>((part1) | 0x80);
	size4: target[3] = static_cast<uint8>((part0 >> 21) | 0x80);
	size3: target[2] = static_cast<uint8>((part0 >> 14) | 0x80);
	size2: target[1] = static_cast<uint8>((part0 >> 7) | 0x80);
	size1: target[0] = static_cast<uint8>((part0) | 0x80);

		target[size - 1] &= 0x7F;
		m_iCurBit += size * 8;
	}
	else // slow path
	{
		while (data > 0x7F)
		{
			WriteUBitLong((data & 0x7F) | 0x80, 8);
			data >>= 7;
		}
		WriteUBitLong(data & 0x7F, 8);
	}
}

//-----------------------------------------------------------------------------
void CBitWrite::WriteSignedVarInt32(int32 data)
{
	WriteVarInt32(bitbuf::ZigZagEncode32(data));
}

//-----------------------------------------------------------------------------
void CBitWrite::WriteSignedVarInt64(int64 data)
{
	WriteVarInt64(bitbuf::ZigZagEncode64(data));
}

//-----------------------------------------------------------------------------
int CBitWrite::ByteSizeVarInt32(uint32 data)
{
	int size = 1;
	while (data > 0x7F) {
		size++;
		data >>= 7;
	}
	return size;
}

//-----------------------------------------------------------------------------
int CBitWrite::ByteSizeVarInt64(uint64 data)
{
	int size = 1;
	while (data > 0x7F) {
		size++;
		data >>= 7;
	}
	return size;
}

//-----------------------------------------------------------------------------
int CBitWrite::ByteSizeSignedVarInt32(int32 data)
{
	return ByteSizeVarInt32(bitbuf::ZigZagEncode32(data));
}

//-----------------------------------------------------------------------------
int CBitWrite::ByteSizeSignedVarInt64(int64 data)
{
	return ByteSizeVarInt64(bitbuf::ZigZagEncode64(data));
}


///////////////////////////////////////////////////////////////////////////////
// Routines for writing coordinates into the buffer (these contain bit-field
// size AND fixed point scaling constants)
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
void CBitWrite::WriteBitAngle(float fAngle, int numbits)
{
	int d;
	unsigned int mask;
	unsigned int shift;

	shift = GetBitForBitnum(numbits);
	mask = shift - 1;

	d = (int)((fAngle / 360.0) * shift);
	d &= mask;

	WriteUBitLong((unsigned int)d, numbits);
}

//-----------------------------------------------------------------------------
void CBitWrite::WriteBitCoord(const float f)
{
#if defined( BB_PROFILING )
	VPROF("CBitWrite::WriteBitCoord");
#endif
	int		signbit = (f <= -COORD_RESOLUTION);
	int		intval = (int)abs(f);
	int		fractval = abs((int)(f * COORD_DENOMINATOR)) & (COORD_DENOMINATOR - 1);

	// Send the bit flags that indicate whether we have an integer part and/or a fraction part.
	WriteOneBit(intval);
	WriteOneBit(fractval);

	if (intval || fractval)
	{
		// Send the sign bit
		WriteOneBit(signbit);

		// Send the integer if we have one.
		if (intval)
		{
			// Adjust the integers from [1..MAX_COORD_VALUE] to [0..MAX_COORD_VALUE-1]
			intval--;
			WriteUBitLong((unsigned int)intval, COORD_INTEGER_BITS);
		}

		// Send the fraction if we have one
		if (fractval)
		{
			WriteUBitLong((unsigned int)fractval, COORD_FRACTIONAL_BITS);
		}
	}
}

//-----------------------------------------------------------------------------
void CBitWrite::WriteBitCoordMP(const float f, bool bIntegral, bool bLowPrecision)
{
#if defined( BB_PROFILING )
	VPROF("CBitWrite::WriteBitCoordMP");
#endif
	int		signbit = (f <= -(bLowPrecision ? COORD_RESOLUTION_LOWPRECISION : COORD_RESOLUTION));
	int		intval = (int)abs(f);
	int		fractval = bLowPrecision ?
		(abs((int)(f * COORD_DENOMINATOR_LOWPRECISION)) & (COORD_DENOMINATOR_LOWPRECISION - 1)) :
		(abs((int)(f * COORD_DENOMINATOR)) & (COORD_DENOMINATOR - 1));

	bool    bInBounds = intval < (1 << COORD_INTEGER_BITS_MP);

	unsigned int bits, numbits;

	if (bIntegral)
	{
		// Integer encoding: in-bounds bit, nonzero bit, optional sign bit + integer value bits
		if (intval)
		{
			// Adjust the integers from [1..MAX_COORD_VALUE] to [0..MAX_COORD_VALUE-1]
			--intval;
			bits = intval * 8 + signbit * 4 + 2 + bInBounds;
			numbits = 3 + (bInBounds ? COORD_INTEGER_BITS_MP : COORD_INTEGER_BITS);
		}
		else
		{
			bits = bInBounds;
			numbits = 2;
		}
	}
	else
	{
		// Float encoding: in-bounds bit, integer bit, sign bit, fraction value bits, optional integer value bits
		if (intval)
		{
			// Adjust the integers from [1..MAX_COORD_VALUE] to [0..MAX_COORD_VALUE-1]
			--intval;
			bits = intval * 8 + signbit * 4 + 2 + bInBounds;
			bits += bInBounds ? (fractval << (3 + COORD_INTEGER_BITS_MP)) : (fractval << (3 + COORD_INTEGER_BITS));
			numbits = 3 + (bInBounds ? COORD_INTEGER_BITS_MP : COORD_INTEGER_BITS)
				+ (bLowPrecision ? COORD_FRACTIONAL_BITS_MP_LOWPRECISION : COORD_FRACTIONAL_BITS);
		}
		else
		{
			bits = fractval * 8 + signbit * 4 + 0 + bInBounds;
			numbits = 3 + (bLowPrecision ? COORD_FRACTIONAL_BITS_MP_LOWPRECISION : COORD_FRACTIONAL_BITS);
		}
	}

	WriteUBitLong(bits, numbits);
}

//-----------------------------------------------------------------------------
void CBitWrite::WriteBitCellCoord(const float f, int bits, EBitCoordType coordType)
{
#if defined( BB_PROFILING )
	VPROF("CBitWrite::WriteBitCellCoord");
#endif
	Assert(f >= 0.0f); // cell coords can't be negative
	Assert(f < (1 << bits));

	bool bIntegral = (coordType == kCW_Integral);
	bool bLowPrecision = (coordType == kCW_LowPrecision);

	int		intval = (int)abs(f);
	int		fractval = bLowPrecision ?
		(abs((int)(f * COORD_DENOMINATOR_LOWPRECISION)) & (COORD_DENOMINATOR_LOWPRECISION - 1)) :
		(abs((int)(f * COORD_DENOMINATOR)) & (COORD_DENOMINATOR - 1));

	if (bIntegral)
	{
		WriteUBitLong((unsigned int)intval, bits);
	}
	else
	{
		WriteUBitLong((unsigned int)intval, bits);
		WriteUBitLong((unsigned int)fractval, bLowPrecision ? COORD_FRACTIONAL_BITS_MP_LOWPRECISION : COORD_FRACTIONAL_BITS);
	}
}

//-----------------------------------------------------------------------------
void CBitWrite::WriteBitVec3Coord(const Vector3D& fa)
{
	int		xflag, yflag, zflag;

	xflag = (fa[0] >= COORD_RESOLUTION) || (fa[0] <= -COORD_RESOLUTION);
	yflag = (fa[1] >= COORD_RESOLUTION) || (fa[1] <= -COORD_RESOLUTION);
	zflag = (fa[2] >= COORD_RESOLUTION) || (fa[2] <= -COORD_RESOLUTION);

	WriteOneBit(xflag);
	WriteOneBit(yflag);
	WriteOneBit(zflag);

	if (xflag)
		WriteBitCoord(fa[0]);
	if (yflag)
		WriteBitCoord(fa[1]);
	if (zflag)
		WriteBitCoord(fa[2]);
}

//-----------------------------------------------------------------------------
void CBitWrite::WriteBitNormal(float f)
{
	int	signbit = (f <= -NORMAL_RESOLUTION);

	// NOTE: Since +/-1 are valid values for a normal, I'm going to encode that as all ones
	unsigned int fractval = abs((int)(f * NORMAL_DENOMINATOR));

	// clamp..
	if (fractval > NORMAL_DENOMINATOR)
		fractval = NORMAL_DENOMINATOR;

	// Send the sign bit
	WriteOneBit(signbit);

	// Send the fractional component
	WriteUBitLong(fractval, NORMAL_FRACTIONAL_BITS);
}

//-----------------------------------------------------------------------------
void CBitWrite::WriteBitVec3Normal(const Vector3D& fa)
{
	int		xflag, yflag;

	xflag = (fa[0] >= NORMAL_RESOLUTION) || (fa[0] <= -NORMAL_RESOLUTION);
	yflag = (fa[1] >= NORMAL_RESOLUTION) || (fa[1] <= -NORMAL_RESOLUTION);

	WriteOneBit(xflag);
	WriteOneBit(yflag);

	if (xflag)
		WriteBitNormal(fa[0]);
	if (yflag)
		WriteBitNormal(fa[1]);

	// Write z sign bit
	int	signbit = (fa[2] <= -NORMAL_RESOLUTION);
	WriteOneBit(signbit);
}

//-----------------------------------------------------------------------------
void CBitWrite::WriteBitAngles(const QAngle& fa)
{
	// FIXME:
	Vector3D tmp(fa.x, fa.y, fa.z);
	WriteBitVec3Coord(tmp);
}


///////////////////////////////////////////////////////////////////////////////
// Routines for writing basic integral types into the buffer
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
void CBitWrite::WriteChar(int val)
{
	WriteSBitLong(val, sizeof(char) << 3);
}

//-----------------------------------------------------------------------------
void CBitWrite::WriteByte(int val)
{
	WriteUBitLong(val, sizeof(unsigned char) << 3);
}

//-----------------------------------------------------------------------------
void CBitWrite::WriteShort(int val)
{
	WriteSBitLong(val, sizeof(short) << 3);
}

//-----------------------------------------------------------------------------
void CBitWrite::WriteWord(int val)
{
	WriteUBitLong(val, sizeof(unsigned short) << 3);
}

//-----------------------------------------------------------------------------
void CBitWrite::WriteLong(long val)
{
	WriteSBitLong(val, sizeof(long) << 3);
}

//-----------------------------------------------------------------------------
void CBitWrite::WriteLongLong(int64 val)
{
	uint* pLongs = (uint*)&val;

	// Insert the two DWORDS according to network endian
	const short endianIndex = 0x0100;
	byte* idx = (byte*)&endianIndex;
	WriteUBitLong(pLongs[*idx++], sizeof(long) << 3);
	WriteUBitLong(pLongs[*idx], sizeof(long) << 3);
}

//-----------------------------------------------------------------------------
void CBitWrite::WriteFloat(float val)
{
	// Pre-swap the float, since WriteBits writes raw data
	LittleFloat(&val, &val);

	WriteBits(&val, sizeof(val) << 3);
}

//-----------------------------------------------------------------------------
bool CBitWrite::WriteBytes(const void* pBuf, int nBytes)
{
	return WriteBits(pBuf, nBytes << 3);
}


///////////////////////////////////////////////////////////////////////////////
// Routines for writing strings into the buffer
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
bool CBitWrite::WriteString(const char* pStr)
{
	if (pStr)
	{
		do
		{
			WriteChar(*pStr);
			++pStr;
		} while (*(pStr - 1) != 0);
	}
	else
	{
		WriteChar(0);
	}

	return !IsOverflowed();
}

//-----------------------------------------------------------------------------
bool CBitWrite::WriteWString(const wchar_t* pStr)
{
	if (pStr)
	{
		do
		{
			WriteShort(*pStr);
			++pStr;
		} while (*(pStr - 1) != 0);
	}
	else
	{
		WriteShort(0);
	}

	return !IsOverflowed();
}
