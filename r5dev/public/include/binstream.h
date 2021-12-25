#pragma once

enum class eStreamFileMode
{
	NONE = 0,
	READ,
	WRITE
};

class CIOStream
{
	std::ofstream writer; // Output file stream.
	std::ifstream reader; // Input file stream.
	std::string svFilePath = ""; // Filepath.
	eStreamFileMode eCurrentMode = eStreamFileMode::NONE; // Current active mode.

public:
	CIOStream();
	~CIOStream();

	bool open(std::string fileFullPath, eStreamFileMode mode);
	void close();

	bool checkWritabilityStatus();
	bool checkReadabilityStatus();

	bool eof();

	//-----------------------------------------------------------------------------
	// Purpose: reads any value from the file (for strings use 'readString(...)' instead)
	//-----------------------------------------------------------------------------
	template<typename T>
	void read(T& value) // Template functions have to be in the header!
	{
		if (checkReadabilityStatus())
		{
			reader.read((char*)&value, sizeof(value));
		}
	}

	//-----------------------------------------------------------------------------
	// Purpose: reads any value from the file and returns it (for strings use 'readString(...)' instead)
	//-----------------------------------------------------------------------------
	template<typename T>
	T readR() // Template functions have to be in the header!
	{
		checkReadabilityStatus();

		T value;
		reader.read((char*)&value, sizeof(value));
		return value;
	}
	std::string readString();

	//-----------------------------------------------------------------------------
	// Purpose: writes any value to the file (for strings use 'writeString(...)' instead)
	//-----------------------------------------------------------------------------
	template<typename T>
	void write(T& value) // Template functions have to be in the header!
	{
		if (!checkWritabilityStatus())
		{
			return;
		}
		writer.write((const char*)&value, sizeof(value));
	}
	void writeString(std::string str);
};
