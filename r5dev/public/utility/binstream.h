#pragma once

class CIOStream
{
public:
	enum class Mode_t
	{
		NONE = 0,
		READ,
		WRITE
	};

	CIOStream();
	CIOStream(const fs::path& fsFileFullPath, Mode_t eMode);
	~CIOStream();

	bool Open(const fs::path& fsFileFullPath, Mode_t eMode);
	void Close();
	void Flush();

	void ComputeFileSize();

	std::streampos GetPosition();
	void SetPosition(std::streampos nOffset);

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
			m_iStream.read(reinterpret_cast<char*>(&tValue), sizeof(tValue));
	}

	//-----------------------------------------------------------------------------
	// Purpose: reads any value from the file with specified size
	//-----------------------------------------------------------------------------
	template<typename T>
	void Read(T& tValue, size_t nSize)
	{
		if (IsReadable())
			m_iStream.read(reinterpret_cast<char*>(&tValue), nSize);
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

		m_iStream.read(reinterpret_cast<char*>(&value), sizeof(value));
		return value;
	}
	string ReadString();

	//-----------------------------------------------------------------------------
	// Purpose: writes any value to the file
	//-----------------------------------------------------------------------------
	template<typename T>
	void Write(T tValue)
	{
		if (!IsWritable())
			return;

		m_oStream.write(reinterpret_cast<const char*>(&tValue), sizeof(tValue));
	}

	//-----------------------------------------------------------------------------
	// Purpose: writes any value to the file with specified size
	//-----------------------------------------------------------------------------
	template<typename T>
	void Write(T tValue, size_t nSize)
	{
		if (!IsWritable())
			return;

		m_oStream.write(reinterpret_cast<const char*>(tValue), nSize);
	}
	void WriteString(string svInput);

private:

	std::streampos  m_nSize;        // Size of ifstream.
	Mode_t          m_eCurrentMode; // Current active mode.
	ofstream        m_oStream;      // Output file stream.
	ifstream        m_iStream;      // Input file stream.
};
