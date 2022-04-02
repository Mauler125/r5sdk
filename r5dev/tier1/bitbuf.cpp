//===========================================================================//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#include "core/stdafx.h"
#include "tier1/bitbuf.h"

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

unsigned char* bf_write::GetData()
{
	return this->m_pData;
}

const unsigned char* bf_write::GetData() const
{
	return this->m_pData;
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
		Assert(false);
	}

	this->m_bOverflow = true;
}