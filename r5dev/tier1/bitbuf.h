#ifndef BITBUF_H
#define BITBUF_H

typedef enum
{
	BITBUFERROR_VALUE_OUT_OF_RANGE = 0, // Tried to write a value with too few bits.
	BITBUFERROR_BUFFER_OVERRUN,         // Was about to overrun a buffer.

	BITBUFERROR_NUM_ERRORS
} BitBufErrorType;

//-----------------------------------------------------------------------------
// Used for serialization
//-----------------------------------------------------------------------------
struct bf_write
{
public:
	// How many bytes are filled in?
	int            GetNumBytesWritten() const;
	int            GetNumBitsWritten() const;
	int            GetMaxNumBits() const;
	int            GetNumBitsLeft() const;
	int            GetNumBytesLeft() const;
	unsigned char* GetData();
	const unsigned char* GetData() const;

	// Has the buffer overflowed?
	bool CheckForOverflow(int nBits);
	bool IsOverflowed() const;
	void SetOverflowFlag();
private:
	unsigned __int8* m_pData;
	int m_nDataBytes;
	int m_nDataBits;
	int m_iCurBit;
	bool m_bOverflow;
	bool m_bAssertOnOverflow;
	const char* m_pDebugName;
};
#endif // BITBUF_H
