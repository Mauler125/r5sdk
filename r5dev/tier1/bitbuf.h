#ifndef BITBUF_H
#define BITBUF_H

typedef enum
{
	BITBUFERROR_VALUE_OUT_OF_RANGE = 0, // Tried to write a value with too few bits.
	BITBUFERROR_BUFFER_OVERRUN,         // Was about to overrun a buffer.

	BITBUFERROR_NUM_ERRORS
} BitBufErrorType;

#define LittleDWord(val) (val)

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
	size_t m_nDataBits;
	size_t m_nDataBytes;
};

class CBitRead : public CBitBuffer
{
public:
	void GrabNextDWord(bool bOverFlowImmediately = false);
	void FetchNext();

	int ReadSBitLong(int numbits);
	uint32 ReadUBitLong(int numbits);

	int ReadByte();
	int ReadChar();
	bool ReadString(char* pStr, int bufLen, bool bLine = false, int* pOutNumChars = nullptr);

	void StartReading(const void* pData, size_t nBytes, size_t iStartBit = 0, size_t nBits = -1);
	bool Seek(size_t nPosition);

	////////////////////////////////////
	uint32_t m_nInBufWord;
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
	// How many bytes are filled in?
	int            GetNumBytesWritten() const;
	int            GetNumBitsWritten() const;
	int            GetMaxNumBits() const;
	int            GetNumBitsLeft() const;
	int            GetNumBytesLeft() const;
	unsigned char* GetData() const;
	const char* GetDebugName() const;

	// Has the buffer overflowed?
	bool CheckForOverflow(int nBits);
	bool IsOverflowed() const;
	void SetOverflowFlag();
private:
	uint8_t* m_pData;
	int m_nDataBytes;
	int m_nDataBits;
	int m_iCurBit;
	bool m_bOverflow;
	bool m_bAssertOnOverflow;
	const char* m_pDebugName;
};
#endif // BITBUF_H
