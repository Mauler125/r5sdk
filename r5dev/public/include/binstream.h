#pragma once

enum class eStreamFileMode
{
	NONE = 0,
	READ,
	WRITE
};

class CIOStream
{
	ofstream writer;              // Output file stream.
	ifstream reader;              // Input file stream.
	string svFilePath;            // Filepath.
	eStreamFileMode eCurrentMode; // Current active mode.

public:
	CIOStream();
	CIOStream(const string& svFileFullPath, eStreamFileMode eMode);
	~CIOStream();

	bool Open(const string& svFileFullPath, eStreamFileMode eMode);
	void Close();

	size_t GetPosition();
	void SetPosition(int64_t nOffset);

	bool IsReadable();
	bool IsWritable() const;

	bool IsEof() const;

	//-----------------------------------------------------------------------------
	// Purpose: reads any value from the file (for strings use 'readString(...)' instead)
	//-----------------------------------------------------------------------------
	template<typename T>
	void Read(T& tValue) // Template functions have to be in the header!
	{
		if (IsReadable())
		{
			reader.read(reinterpret_cast<char*>(&tValue), sizeof(tValue));
		}
	}

	//-----------------------------------------------------------------------------
	// Purpose: reads any value from the file and returns it (for strings use 'readString(...)' instead)
	//-----------------------------------------------------------------------------
	template<typename T>
	T Read() // Template functions have to be in the header!
	{
		IsReadable();

		T value;
		reader.read(reinterpret_cast<char*>(&value), sizeof(value));
		return value;
	}
	string ReadString();

	//-----------------------------------------------------------------------------
	// Purpose: writes any value to the file (for strings use 'writeString(...)' instead)
	//-----------------------------------------------------------------------------
	template<typename T>
	void Write(T tValue) // Template functions have to be in the header!
	{
		if (!IsWritable())
		{
			return;
		}
		writer.write(reinterpret_cast<const char*>(&tValue), sizeof(tValue));
	}
	void WriteString(string svInput);
};
