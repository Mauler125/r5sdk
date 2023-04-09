//===========================================================================//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#include "core/stdafx.h"
#include "tier1/bitbuf.h"

const uint32 s_nMaskTable[33] = {
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


CBitBuffer::CBitBuffer(void)
{
	m_bOverflow = false;
	m_pDebugName = NULL;
	m_nDataBits = -1;
	m_nDataBytes = 0;
}

void CBitBuffer::SetDebugName(const char* pName)
{
	m_pDebugName = pName;
}

const char* CBitBuffer::GetDebugName() const
{
	return m_pDebugName;
}

bool CBitBuffer::IsOverflowed() const
{
	return m_bOverflow;
}

void CBitBuffer::SetOverflowFlag()
{
	m_bOverflow = true;
}

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

void CBitRead::FetchNext()
{
	m_nBitsAvail = 32;
	GrabNextDWord(false);
}

uint32 CBitRead::ReadUBitLong(int numbits)
{
	if (m_nBitsAvail >= numbits)
	{
		unsigned int nRet = m_nInBufWord & s_nMaskTable[numbits];
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

		nRet |= ((m_nInBufWord & s_nMaskTable[numbits]) << m_nBitsAvail);
		m_nBitsAvail = 32 - numbits;
		m_nInBufWord >>= numbits;

		return nRet;
	}
}

int CBitRead::ReadSBitLong(int numbits)
{
	int nRet = ReadUBitLong(numbits);
	return (nRet << (32 - numbits)) >> (32 - numbits);
}

int CBitRead::ReadByte()
{
	return ReadSBitLong(sizeof(unsigned char) << 3);
}

int CBitRead::ReadChar()
{
	return ReadSBitLong(sizeof(char) << 3);
}

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

bool CBitRead::Seek(int64_t nPosition)
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

void CBitRead::StartReading(const void* pData, size_t nBytes, int64_t iStartBit, int64_t nBits)
{
	// Make sure it's dword aligned and padded.
	assert((int64_t(pData) & 3) == 0);
	m_pData = (uint32*)pData;
	m_pDataIn = m_pData;
	m_nDataBytes = nBytes;

	if (nBits == -1)
	{
		m_nDataBits = nBytes << 3;
	}
	else
	{
		assert(nBits <= int64_t(nBytes * 8));
		m_nDataBits = nBits;
	}
	m_bOverflow = false;
	m_pBufferEnd = reinterpret_cast<uint32 const*> (reinterpret_cast<uint8 const*> (m_pData) + nBytes);
	if (m_pData)
		Seek(iStartBit);

}

inline int BitByte(int bits)
{
	// return PAD_NUMBER( bits, 8 ) >> 3;
	return (bits + 7) >> 3;
}

bool bf_write::IsOverflowed() const
{
	return this->m_bOverflow;
}

int bf_write::GetNumBytesWritten() const
{
	return BitByte(this->m_iCurBit);
}

int bf_write::GetNumBitsWritten() const
{
	return this->m_iCurBit;
}

int bf_write::GetMaxNumBits() const
{
	return this->m_nDataBits;
}

int bf_write::GetNumBitsLeft() const
{
	return this->m_nDataBits - m_iCurBit;
}

int bf_write::GetNumBytesLeft() const
{
	return this->GetNumBitsLeft() >> 3;
}

unsigned char* bf_write::GetData() const
{
	return this->m_pData;
}

const char* bf_write::GetDebugName() const
{
	return this->m_pDebugName;
}

bool bf_write::CheckForOverflow(int nBits)
{
	if (this->m_iCurBit + nBits > this->m_nDataBits)
	{
		this->SetOverflowFlag();
	}

	return this->m_bOverflow;
}

void bf_write::SetOverflowFlag()
{
	if (this->m_bAssertOnOverflow)
	{
		assert(false);
	}

	this->m_bOverflow = true;
}