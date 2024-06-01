//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: buffer serialization/deserialization.
//
// $NoKeywords: $
// NOTE: bf_read is guaranteed to return zeros if it overflows.
//===========================================================================//

#ifndef BITBUF_H
#define BITBUF_H
#include "mathlib/swap.h"

//-----------------------------------------------------------------------------
// You can define a handler function that will be called in case of 
// out-of-range values and overruns here.
//
// NOTE: the handler is only called in debug mode.
//
// Call SetBitBufErrorHandler to install a handler.
//-----------------------------------------------------------------------------
typedef enum
{
	BITBUFERROR_VALUE_OUT_OF_RANGE = 0, // Tried to write a value with too few bits.
	BITBUFERROR_BUFFER_OVERRUN,         // Was about to overrun a buffer.

	BITBUFERROR_NUM_ERRORS
} BitBufErrorType;
typedef void (*BitBufErrorHandler)(BitBufErrorType errorType, const char* pDebugName);

#if defined( _DEBUG )
extern void InternalBitBufErrorHandler(BitBufErrorType errorType, const char* pDebugName);
#define CallErrorHandler( errorType, pDebugName ) InternalBitBufErrorHandler( errorType, pDebugName );
#else
#define CallErrorHandler( errorType, pDebugName )
#endif

// Use this to install the error handler. Call with NULL to uninstall your error handler.
void SetBitBufErrorHandler(BitBufErrorHandler fn);

//-----------------------------------------------------------------------------
// Helpers.
//-----------------------------------------------------------------------------

inline int BitByte(int bits)
{
	// return PAD_NUMBER( bits, 8 ) >> 3;
	return (bits + 7) >> 3;
}

//-----------------------------------------------------------------------------
// namespaced helpers
//-----------------------------------------------------------------------------
namespace bitbuf
{
	// ZigZag Transform:  Encodes signed integers so that they can be
	// effectively used with varint encoding.
	//
	// varint operates on unsigned integers, encoding smaller numbers into
	// fewer bytes.  If you try to use it on a signed integer, it will treat
	// this number as a very large unsigned integer, which means that even
	// small signed numbers like -1 will take the maximum number of bytes
	// (10) to encode.  ZigZagEncode() maps signed integers to unsigned
	// in such a way that those with a small absolute value will have smaller
	// encoded values, making them appropriate for encoding using varint.
	//
	//       int32 ->     uint32
	// -------------------------
	//           0 ->          0
	//          -1 ->          1
	//           1 ->          2
	//          -2 ->          3
	//         ... ->        ...
	//  2147483647 -> 4294967294
	// -2147483648 -> 4294967295
	//
	//        >> encode >>
	//        << decode <<

	inline uint32 ZigZagEncode32(int32 n)
	{
		// Note:  the right-shift must be arithmetic
		return(n << 1) ^ (n >> 31);
	}

	inline int32 ZigZagDecode32(uint32 n)
	{
		return(n >> 1) ^ -static_cast<int32>(n & 1);
	}

	inline uint64 ZigZagEncode64(int64 n)
	{
		// Note:  the right-shift must be arithmetic
		return(n << 1) ^ (n >> 63);
	}

	inline int64 ZigZagDecode64(uint64 n)
	{
		return(n >> 1) ^ -static_cast<int64>(n & 1);
	}

	const int kMaxVarint32Bytes = 5;
	const int kMaxVarint64Bytes = 10;
}

//-----------------------------------------------------------------------------
enum EBitCoordType
{
	kCW_None,
	kCW_LowPrecision,
	kCW_Integral
};

//-----------------------------------------------------------------------------
class CBitBuffer
{
public:
	CBitBuffer(void);

	FORCEINLINE void        SetDebugName(const char* pName) { m_pDebugName = pName; }
	FORCEINLINE const char* GetDebugName()            const { return m_pDebugName; }
	FORCEINLINE bool        IsOverflowed()            const { return m_bOverflow; }
	FORCEINLINE void        SetOverflowFlag()               { m_bOverflow = true; }

	////////////////////////////////////
	const char* m_pDebugName;
	bool m_bOverflow;
	ssize_t m_nDataBits;
	size_t m_nDataBytes;

	static const uint32 s_nMaskTable[33]; // 0 1 3 7 15 ..
};

//-----------------------------------------------------------------------------
class CBitRead : public CBitBuffer
{
public:
	CBitRead(const void* pData, size_t nBytes, ssize_t nBits = -1);
	CBitRead(const char* pDebugName, const void* pData, size_t nBytes, ssize_t nBits = -1);
	CBitRead(void) : CBitBuffer()
	{
	}

	void StartReading(const void* pData, size_t nBytes, ssize_t iStartBit = 0, ssize_t nBits = -1);

	FORCEINLINE ssize_t Tell(void) const;
	bool Seek(ssize_t nPosition);

	FORCEINLINE bool SeekRelative(ssize_t nOffset)
	{
		return Seek(GetNumBitsRead() + nOffset);
	}

	FORCEINLINE unsigned char const* GetBasePointer()
	{
		return reinterpret_cast<unsigned char const*>(m_pData);
	}

	FORCEINLINE ssize_t GetNumBitsRead(void)     const { return Tell(); };
	FORCEINLINE ssize_t GetNumBytesRead(void)    const { return ((GetNumBitsRead() + 7) >> 3); }

	FORCEINLINE ssize_t GetNumBitsLeft(void)     const { return m_nDataBits - GetNumBitsRead(); }
	FORCEINLINE ssize_t GetNumBytesLeft(void)    const { return GetNumBitsLeft() >> 3; }

	FORCEINLINE size_t TotalBytesAvailable(void) const { return m_nDataBytes; }

	FORCEINLINE void GrabNextDWord(bool bOverFlowImmediately = false);
	FORCEINLINE void FetchNext(void);

	FORCEINLINE unsigned int ReadUBitLong(int numbits);
	FORCEINLINE int          ReadSBitLong(int numbits);
	FORCEINLINE unsigned int PeekUBitLong(int numbits);

	FORCEINLINE unsigned int ReadUBitVar(void);

	FORCEINLINE float ReadBitFloat(void)
	{
		uint32 nvalue = ReadUBitLong(32);
		return *((float*)&nvalue);
	}


	float ReadBitCoord();
	float ReadBitCoordMP(EBitCoordType coordType);
	float ReadBitCellCoord(int bits, EBitCoordType coordType);
	float ReadBitNormal();
	void ReadBitVec3Coord(Vector3D& fa);
	void ReadBitVec3Normal(Vector3D& fa);
	void ReadBitAngles(QAngle& fa);
	float ReadBitAngle(int numbits);

	// returns 0 or 1.
	FORCEINLINE int	ReadOneBit(void);

	FORCEINLINE int ReadChar(void)  { return ReadSBitLong(sizeof(char) << 3); }
	FORCEINLINE int ReadByte(void)  { return ReadSBitLong(sizeof(unsigned char) << 3); }
	FORCEINLINE int ReadShort(void) { return ReadUBitLong(sizeof(short) << 3); }
	FORCEINLINE int ReadWord(void)  { return ReadUBitLong(sizeof(unsigned short) << 3); }
	FORCEINLINE int ReadLong(void)  { return ReadUBitLong(sizeof(int32) << 3); }
	FORCEINLINE float ReadFloat(void)
	{
		uint32 nUval = ReadUBitLong(sizeof(int32) << 3);
		return *((float*)&nUval);
	}

	// reads a signed 64-bit integer from the buffer
	int64           ReadLongLong(void);

	// reads a varint encoded integer
	uint32          ReadVarInt32();
	uint64          ReadVarInt64();
	int32           ReadSignedVarInt32() { return bitbuf::ZigZagDecode32(ReadVarInt32()); }
	int64           ReadSignedVarInt64() { return bitbuf::ZigZagDecode64(ReadVarInt64()); }

	void            ReadBits(void* pOutData, int nBits);

	// Helper 'safe' template function that infers the size of the destination
	// array. This version of the function should be preferred.
	// Usage: char databuffer[100];
	//        ReadBitsClamped( dataBuffer, msg->m_nLength );
	template <typename T, int N>
	FORCEINLINE int ReadBitsClamped(T (&pOut)[N], int nBits)
	{
		const int outSizeBytes = N * sizeof(T);
		const int outSizeBits = outSizeBytes * 8;

		if (nBits > outSizeBits)
			nBits = outSizeBits;

		ReadBits(pOut, nBits);
		return nBits;
	}

	bool            ReadBytes(void* pOut, int nBytes);


	// Returns false if bufLen isn't large enough to hold the
	// string in the buffer.
	//
	// Always reads to the end of the string (so you can read the
	// next piece of data waiting).
	//
	// If bLine is true, it stops when it reaches a '\n' or a null-terminator.
	//
	// pStr is always null-terminated (unless bufLen is 0).
	//
	// pOutNumChars is set to the number of characters left in pStr when the routine is 
	// complete (this will never exceed bufLen-1).
	//
	bool            ReadString(char* pStr, int bufLen, bool bLine = false, int* pOutNumChars = NULL);
	bool            ReadWString(wchar_t* pStr, int bufLen, bool bLine = false, int* pOutNumChars = NULL);

	// Reads a string and allocates memory for it. If the string in the buffer
	// is > 2048 bytes, then pOverflow is set to true (if it's not NULL).
	char*           ReadAndAllocateString(bool *pOverflow = NULL);

	////////////////////////////////////
	uint32 m_nInBufWord;
	int m_nBitsAvail;
	const uint32* m_pDataIn;
	const uint32* m_pBufferEnd;
	const uint32* m_pData;
};

//-----------------------------------------------------------------------------
class CBitWrite
{
public:
	CBitWrite();

	// nMaxBits can be used as the number of bits in the buffer. 
	// It must be <= nBytes*8. If you leave it at -1, then it's set to nBytes * 8.
	CBitWrite(void* pData, int nBytes, int nMaxBits = -1);
	CBitWrite(const char* pDebugName, void* pData, int nBytes, int nMaxBits = -1);

	// Restart buffer writing.
	inline void     Reset() { m_iCurBit = 0; m_bOverflow = false; }
	inline void     SeekToBit(int bitPos) { m_iCurBit = bitPos; }

	void StartWriting(void* pData, int nBytes, int iStartBit = 0, int nMaxBits = -1);

	// Bit functions.
	void            WriteOneBit(int nValue);
	void            WriteOneBitNoCheck(int nValue);
	void            WriteOneBitAt(int iBit, int nValue);

	// Write signed or unsigned. Range is only checked in debug.
	void            WriteUBitLong(unsigned int curData, int numbits, bool bCheckRange = false);
	void            WriteSBitLong(int data, int numbits);

	// Tell it whether or not the data is unsigned. If it's signed,
	// cast to unsigned before passing in (it will cast back inside).
	void            WriteBitLong(unsigned int data, int numbits, bool bSigned);

	// Write a list of bits in.
	bool            WriteBits(const void* pIn, int nBits);

	// copy the bits straight out of pIn. This seeks pIn forward by nBits,
	// returns false if this buffer or the read buffer overflows
	bool            WriteBitsFromBuffer(class bf_read *pIn, int nBits);

	// writes an unsigned integer with variable bit length
	void            WriteUBitVar(unsigned int data);

	// writes a varint encoded integer
	void            WriteVarInt32(uint32 data);
	void            WriteVarInt64(uint64 data);
	void            WriteSignedVarInt32(int32 data);
	void            WriteSignedVarInt64(int64 data);
	int             ByteSizeVarInt32(uint32 data);
	int             ByteSizeVarInt64(uint64 data);
	int             ByteSizeSignedVarInt32(int32 data);
	int             ByteSizeSignedVarInt64(int64 data);

	// writes an angle or vector encoded
	void            WriteBitAngle(float fAngle, int numbits);
	void            WriteBitCoord(const float f);
	void            WriteBitCoordMP(const float f, bool bIntegral, bool bLowPrecision);
	void            WriteBitCellCoord(const float f, int bits, EBitCoordType coordType);
	void            WriteBitFloat(float val);
	void            WriteBitVec3Coord(const Vector3D& fa);
	void            WriteBitNormal(float f);
	void            WriteBitVec3Normal(const Vector3D& fa);
	void            WriteBitAngles(const QAngle& fa);

	// byte functions
	void            WriteChar(int val);
	void            WriteByte(int val);
	void            WriteShort(int val);
	void            WriteWord(int val);
	void            WriteLong(long val);
	void            WriteLongLong(int64 val);
	void            WriteFloat(float val);
	bool            WriteBytes(const void* pBuf, int nBytes);

	// Returns false if it overflows the buffer.
	bool            WriteString(const char* pStr);
	bool            WriteWString(const wchar_t* pStr);

	// How many bytes are filled in?
	FORCEINLINE int GetNumBytesWritten() const { return BitByte(this->m_iCurBit); }
	FORCEINLINE int GetNumBitsWritten()  const { return this->m_iCurBit; }
	FORCEINLINE int GetMaxNumBits()      const { return this->m_nDataBits; }
	FORCEINLINE int GetNumBitsLeft()     const { return this->m_nDataBits - m_iCurBit; }
	FORCEINLINE int GetNumBytesLeft()    const { return this->GetNumBitsLeft() >> 3; }

	FORCEINLINE unsigned char* GetData()             { return this->m_pData; }
	FORCEINLINE const unsigned char* GetData() const { return this->m_pData; }

	FORCEINLINE const char* GetDebugName()                 const { return this->m_pDebugName; }
	FORCEINLINE void        SetDebugName(const char* pDebugName) { m_pDebugName = pDebugName; }

	// Has the buffer overflowed?
	bool CheckForOverflow(int nBits);
	void SetOverflowFlag();

	FORCEINLINE bool IsOverflowed() const { return this->m_bOverflow; }

private:
	// The current buffer.
	unsigned char*          m_pData;
	int                     m_nDataBytes;
	int                     m_nDataBits;

	// Where we are in the buffer.
	int                     m_iCurBit;

	// Errors?
	bool                    m_bOverflow;
	bool                    m_bAssertOnOverflow;
	const char*             m_pDebugName;
};

//-----------------------------------------------------------------------------
// Used for unserialization
//-----------------------------------------------------------------------------
class bf_read : public CBitRead
{
public:
	bf_read(const void* pData, int nBytes, int nBits = -1) 
		: CBitRead(pData, nBytes, nBits) {}

	bf_read(const char* pDebugName, void* pData, int nBytes, int nMaxBits = -1) 
		: CBitRead(pDebugName, pData, nBytes, nMaxBits) {}

	bf_read(void) : CBitRead()
	{}
};

//-----------------------------------------------------------------------------
// Used for serialization
//-----------------------------------------------------------------------------
class bf_write : public CBitWrite
{
public:
	bf_write(void* pData, int nBytes, int nMaxBits = -1) 
		: CBitWrite(pData, nBytes, nMaxBits) {}

	bf_write(const char* pDebugName, void* pData, int nBytes, int nMaxBits = -1) 
		: CBitWrite(pDebugName, pData, nBytes, nMaxBits) {}

	bf_write(void) : CBitWrite()
	{}
};


///////////////////////////////////////////////////////////////////////////////
// Routines for getting positions and grabbing next data in the read buffer
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
FORCEINLINE ssize_t CBitRead::Tell(void) const
{
	if (!m_pData) // pesky null ptr bitbufs. these happen.
	{
		Assert(m_pData);
		return 0;
	}

	ssize_t nCurOfs = int64(((intp(m_pDataIn) - intp(m_pData)) / 4) - 1);
	nCurOfs *= 32;
	nCurOfs += (32 - m_nBitsAvail);
	ssize_t nAdjust = 8 * (m_nDataBytes & 3);

	return MIN(nCurOfs + nAdjust, m_nDataBits);
}

//-----------------------------------------------------------------------------
FORCEINLINE void CBitRead::GrabNextDWord(bool bOverFlowImmediately)
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
			if (!IsOverflowed())
				SetOverflowFlag();

			m_nInBufWord = 0;
		}
		else
		{
			assert(reinterpret_cast<uintp>(m_pDataIn) + 3 < reinterpret_cast<uintp>(m_pBufferEnd));
			m_nInBufWord = LittleDWord(*(m_pDataIn++));
		}
	}
}

//-----------------------------------------------------------------------------
FORCEINLINE void CBitRead::FetchNext()
{
	m_nBitsAvail = 32;
	GrabNextDWord(false);
}

//-----------------------------------------------------------------------------
FORCEINLINE int CBitRead::ReadOneBit(void)
{
	int nRet = m_nInBufWord & 1;
	if (--m_nBitsAvail == 0)
	{
		FetchNext();
	}
	else
		m_nInBufWord >>= 1;
	return nRet;
}


///////////////////////////////////////////////////////////////////////////////
// Routines for reading encoded integers from the buffer
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// reads an unsigned integer from the buffer
FORCEINLINE unsigned int CBitRead::ReadUBitLong(int numbits)
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

//-----------------------------------------------------------------------------
// reads a signed integer from the buffer
FORCEINLINE int CBitRead::ReadSBitLong(int numbits)
{
	int nRet = ReadUBitLong(numbits);
	return (nRet << (32 - numbits)) >> (32 - numbits);
}

//-----------------------------------------------------------------------------
// peeks an unsigned integer from the buffer
FORCEINLINE unsigned int CBitRead::PeekUBitLong(int numbits)
{
	int nSaveBA = m_nBitsAvail;
	uint32_t nSaveW = m_nInBufWord;
	uint32 const* pSaveP = m_pDataIn;
	unsigned int nRet = ReadUBitLong(numbits);
	m_nBitsAvail = nSaveBA;
	m_nInBufWord = nSaveW;
	m_pDataIn = pSaveP;
	return nRet;
}

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable : 4715)								// disable warning on not all cases
															// returning a value. throwing default:
															// in measurably reduces perf in bit
															// packing benchmark
#endif
//-----------------------------------------------------------------------------
// reads an unsigned integer with variable bit length
FORCEINLINE unsigned int CBitRead::ReadUBitVar(void)
{
	unsigned int ret = ReadUBitLong(6);
	switch (ret & (16 | 32))
	{
	case 16:
		ret = (ret & 15) | (ReadUBitLong(4) << 4);
		Assert(ret >= 16);
		break;

	case 32:
		ret = (ret & 15) | (ReadUBitLong(8) << 4);
		Assert(ret >= 256);
		break;
	case 48:
		ret = (ret & 15) | (ReadUBitLong(32 - 4) << 4);
		Assert(ret >= 4096);
		break;
	}
	return ret;
}
#ifdef _WIN32
#pragma warning(pop)
#endif


///////////////////////////////////////////////////////////////////////////////
// Routines for bit level writing operations
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// writes a bit to the buffer without checking for overflow
FORCEINLINE void CBitWrite::WriteOneBitNoCheck(int nValue)
{
	if (nValue)
		m_pData[m_iCurBit >> 3] |= (1 << (m_iCurBit & 7));
	else
		m_pData[m_iCurBit >> 3] &= ~(1 << (m_iCurBit & 7));

	++m_iCurBit;
}

//-----------------------------------------------------------------------------
// writes a bit to the buffer
FORCEINLINE void CBitWrite::WriteOneBit(int nValue)
{
	if (!CheckForOverflow(1))
		WriteOneBitNoCheck(nValue);
}

//-----------------------------------------------------------------------------
// writes a bit to the buffer at a specific bit index
FORCEINLINE void	CBitWrite::WriteOneBitAt(int iBit, int nValue)
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


///////////////////////////////////////////////////////////////////////////////
// Routines for writing integers into the buffer
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// writes an unsigned integer with variable bit length
FORCEINLINE void CBitWrite::WriteUBitVar(unsigned int data)
{
	/* Reference:
	if ( data < 0x10u )
		WriteUBitLong( 0, 2 ), WriteUBitLong( data, 4 );
	else if ( data < 0x100u )
		WriteUBitLong( 1, 2 ), WriteUBitLong( data, 8 );
	else if ( data < 0x1000u )
		WriteUBitLong( 2, 2 ), WriteUBitLong( data, 12 );
	else
		WriteUBitLong( 3, 2 ), WriteUBitLong( data, 32 );
	*/
	// a < b ? -1 : 0 translates into a CMP, SBB instruction pair
	// with no flow control. should also be branchless on consoles.
	int n = (data < 0x10u ? -1 : 0) + (data < 0x100u ? -1 : 0) + (data < 0x1000u ? -1 : 0);
	WriteUBitLong(data * 4 + n + 3, 6 + n * 4 + 12);
	if (data >= 0x1000u)
	{
		WriteUBitLong(data >> 16, 16);
	}
}

//-----------------------------------------------------------------------------
// write raw IEEE float bits in little endian form
FORCEINLINE void CBitWrite::WriteBitFloat(float val)
{
	long intVal;

	Assert(sizeof(long) == sizeof(float));
	Assert(sizeof(float) == 4);

	intVal = *((long*)&val);
	WriteUBitLong(intVal, 32);
}

#endif // BITBUF_H
