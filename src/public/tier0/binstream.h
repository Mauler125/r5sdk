#pragma once

class CIOStream
{
public:
	enum Mode_t
	{
		NONE = 0,
		READ = std::ios::in,
		WRITE = std::ios::out,
		BINARY = std::ios::binary,
	};

	CIOStream();
	~CIOStream();

	bool Open(const char* const pFilePath, const int nFlags);
	void Close();
	void Flush();

	std::streampos TellGet();
	std::streampos TellPut();

	void SeekGet(const std::streampos nOffset);
	void SeekPut(const std::streampos nOffset);
	void Seek(const std::streampos nOffset);

	const std::filebuf* GetData() const;
	const std::streampos GetSize() const;

	bool IsReadable();
	bool IsWritable() const;

	bool IsEof() const;

	//-----------------------------------------------------------------------------
	// Purpose: reads any value from the file
	//-----------------------------------------------------------------------------
	template<typename T>
	void Read(T& tValue)
	{
		if (IsReadable())
			m_Stream.read(reinterpret_cast<char*>(&tValue), sizeof(tValue));
	}

	//-----------------------------------------------------------------------------
	// Purpose: reads any value from the file with specified size
	//-----------------------------------------------------------------------------
	template<typename T>
	void Read(T* tValue, const size_t nSize)
	{
		if (IsReadable())
			m_Stream.read(reinterpret_cast<char*>(tValue), nSize);
	}
	template<typename T>
	void Read(T& tValue, const size_t nSize)
	{
		if (IsReadable())
			m_Stream.read(reinterpret_cast<char*>(&tValue), nSize);
	}

	//-----------------------------------------------------------------------------
	// Purpose: reads any value from the file and returns it
	//-----------------------------------------------------------------------------
	template<typename T>
	T Read()
	{
		T value{};
		if (!IsReadable())
			return value;

		m_Stream.read(reinterpret_cast<char*>(&value), sizeof(value));
		return value;
	}
	bool ReadString(std::string& svOut);
	bool ReadString(char* const pBuf, const size_t nLen);

	//-----------------------------------------------------------------------------
	// Purpose: writes any value to the file
	//-----------------------------------------------------------------------------
	template<typename T>
	void Write(T tValue)
	{
		if (!IsWritable())
			return;

		m_Stream.write(reinterpret_cast<const char*>(&tValue), sizeof(tValue));
		m_nSize += sizeof(tValue);
	}

	//-----------------------------------------------------------------------------
	// Purpose: writes any value to the file with specified size
	//-----------------------------------------------------------------------------
	template<typename T>
	void Write(T* tValue, size_t nSize)
	{
		if (!IsWritable())
			return;

		m_Stream.write(reinterpret_cast<const char*>(tValue), nSize);
		m_nSize += nSize;
	}
	bool WriteString(const std::string& svInput);

private:
	std::streampos  m_nSize;  // File size.
	int             m_nFlags; // Stream flags.
	std::fstream    m_Stream; // I/O stream.
};
