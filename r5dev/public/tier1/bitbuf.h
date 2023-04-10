//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: buffer serialization/deserialization.
//
// $NoKeywords: $
// NOTE: bf_read is guaranteed to return zeros if it overflows.
//===========================================================================//

#ifndef BITBUF_H
#define BITBUF_H

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

	const int kMaxVarintBytes = 10;
	const int kMaxVarint32Bytes = 5;
}

//-----------------------------------------------------------------------------
// Used for serialization
//-----------------------------------------------------------------------------
class CBitBuffer
{
public:
	CBitBuffer(void);
	void SetDebugName(const char* pName);
	const char* GetDebugName() const;
	bool IsOverflowed() const;
	void SetOverflowFlag();

	////////////////////////////////////
	const char* m_pDebugName;
	uint8_t m_bOverflow;
	int64_t m_nDataBits;
	size_t m_nDataBytes;
};

class CBitRead : public CBitBuffer
{
public:
	CBitRead(const void* pData, int nBytes, int nBits = -1);
	CBitRead(const char* pDebugName, const void* pData, int nBytes, int nBits = -1);
	CBitRead(void) : CBitBuffer()
	{
	}

	void StartReading(const void* pData, size_t nBytes, int64 iStartBit = 0, int64 nBits = -1);
	bool Seek(int64 nPosition);

	void GrabNextDWord(bool bOverFlowImmediately = false);
	void FetchNext();

	int ReadSBitLong(int numbits);
	uint32 ReadUBitLong(int numbits);

	int ReadByte();
	int ReadChar();
	bool ReadString(char* pStr, int bufLen, bool bLine = false, int* pOutNumChars = nullptr);

	////////////////////////////////////
	uint32 m_nInBufWord;
	int m_nBitsAvail;
	const uint32* m_pDataIn;
	const uint32* m_pBufferEnd;
	const uint32* m_pData;
};

class bf_read : public CBitRead
{
public:

};

struct bf_write
{
public:
	bf_write();

	// nMaxBits can be used as the number of bits in the buffer. 
	// It must be <= nBytes*8. If you leave it at -1, then it's set to nBytes * 8.
	bf_write(void* pData, int nBytes, int nMaxBits = -1);
	bf_write(const char* pDebugName, void* pData, int nBytes, int nMaxBits = -1);

	// Restart buffer writing.
	void           Reset();
	void           SeekToBit(int bitPos);

	void StartWriting(void* pData, int nBytes, int iStartBit = 0, int nMaxBits = -1);

	// Bit functions.
	void           WriteOneBit(int nValue);
	void           WriteOneBitNoCheck(int nValue);
	void           WriteOneBitAt(int iBit, int nValue);

	// Write signed or unsigned. Range is only checked in debug.
	void           WriteUBitLong(unsigned int curData, int numbits, bool bCheckRange = false);
	void           WriteSBitLong(int data, int numbits);

	// Tell it whether or not the data is unsigned. If it's signed,
	// cast to unsigned before passing in (it will cast back inside).
	void           WriteBitLong(unsigned int data, int numbits, bool bSigned);

	// Write a list of bits in.
	bool           WriteBits(const void* pIn, int nBits);

	// How many bytes are filled in?
	int            GetNumBytesWritten() const;
	int            GetNumBitsWritten() const;
	int            GetMaxNumBits() const;
	int            GetNumBitsLeft() const;
	int            GetNumBytesLeft() const;
	unsigned char* GetData() const;

	const char* GetDebugName() const;
	void        SetDebugName(const char* pDebugName);

	// Has the buffer overflowed?
	bool CheckForOverflow(int nBits);
	bool IsOverflowed() const;
	void SetOverflowFlag();
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
#endif // BITBUF_H
