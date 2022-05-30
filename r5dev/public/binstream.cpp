#include "core/stdafx.h"
#include "public/include/binstream.h"

//-----------------------------------------------------------------------------
// Purpose: CIOStream constructors
//-----------------------------------------------------------------------------
CIOStream::CIOStream()
{
	eCurrentMode = eStreamFileMode::NONE;
}
CIOStream::CIOStream(const string& svFileFullPath, eStreamFileMode eMode)
{
	Open(svFileFullPath, eMode);
}

//-----------------------------------------------------------------------------
// Purpose: CIOStream destructor
//-----------------------------------------------------------------------------
CIOStream::~CIOStream()
{
	if (writer.is_open())
	{
		writer.close();
	}

	if (reader.is_open())
	{
		reader.close();
	}
}

//-----------------------------------------------------------------------------
// Purpose: opens the file in specified mode
// Input  : fileFullPath - mode
// Output : true if operation is successfull
//-----------------------------------------------------------------------------
bool CIOStream::Open(const string& svFileFullPath, eStreamFileMode eMode)
{
	svFilePath = svFileFullPath;

	if (eMode == eStreamFileMode::WRITE)
	{
		eCurrentMode = eMode;

		// check if we had a previously opened file to close it
		if (writer.is_open())
		{
			writer.close();
		}

		writer.open(svFilePath.c_str(), std::ios::binary);
		if (!writer.is_open())
		{
			Error(eDLL_T::FS, "Error opening file '%s' for write operation!\n", svFilePath.c_str());
			eCurrentMode = eStreamFileMode::NONE;
		}
	}
	// Read mode
	else if (eMode == eStreamFileMode::READ)
	{
		eCurrentMode = eMode;

		// check if we had a previously opened file to close it
		if (reader.is_open())
		{
			reader.close();
		}

		reader.open(svFilePath.c_str(), std::ios::binary);
		if (!reader.is_open())
		{
			Error(eDLL_T::FS, "Error opening file '%s' for read operation!\n", svFilePath.c_str());
			eCurrentMode = eStreamFileMode::NONE;
		}
	}

	// if the mode is still the NONE -> we failed
	return eCurrentMode == eStreamFileMode::NONE ? false : true;
}

//-----------------------------------------------------------------------------
// Purpose: closes the stream
//-----------------------------------------------------------------------------
void CIOStream::Close()
{
	if (eCurrentMode == eStreamFileMode::WRITE)
	{
		writer.close();
	}
	else if (eCurrentMode == eStreamFileMode::READ)
	{
		reader.close();
	}
}

//-----------------------------------------------------------------------------
// Purpose: gets the possition of the current character in the stream
//-----------------------------------------------------------------------------
size_t CIOStream::GetPosition()
{
	switch (eCurrentMode)
	{
	case eStreamFileMode::READ:
		return reader.tellg();
		break;
	case eStreamFileMode::WRITE:
		return writer.tellp();
		break;
	default:
		return 0i64;
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets the possition of the current character in the stream
// Input  : nOffset - 
//-----------------------------------------------------------------------------
void CIOStream::SetPosition(int64_t nOffset)
{
	switch (eCurrentMode)
	{
	case eStreamFileMode::READ:
		reader.seekg(nOffset);
		break;
	case eStreamFileMode::WRITE:
		writer.seekp(nOffset);
		break;
	default:
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: checks if we are able to read the file
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CIOStream::IsReadable()
{
	if (eCurrentMode != eStreamFileMode::READ)
	{
		Error(eDLL_T::FS, "Error: StreamFileMode doesn't match required mode for read operation.\n");
		return false;
	}

	// check if we hit the end of the file.
	if (reader.eof())
	{
		Error(eDLL_T::FS, "Error: trying to read past EOF.\n");
		reader.close();
		eCurrentMode = eStreamFileMode::NONE;
		return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: checks if we are able to write to file
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CIOStream::IsWritable() const
{
	if (eCurrentMode != eStreamFileMode::WRITE)
	{
		Error(eDLL_T::FS, "Error: StreamFileMode doesn't match required mode for write operation.\n");
		return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: checks if we hit the end of file
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CIOStream::IsEof() const
{
	return reader.eof();
}

//-----------------------------------------------------------------------------
// Purpose: reads a string from the file and returns it
// Output : string
//-----------------------------------------------------------------------------
string CIOStream::ReadString()
{
	if (IsReadable())
	{
		char c;
		string result = "";
		while (!reader.eof() && (c = Read<char>()) != '\0')
		{
			result += c;
		}

		return result;
	}
	return "";
}

//-----------------------------------------------------------------------------
// Purpose: writes a string to the file
//-----------------------------------------------------------------------------
void CIOStream::WriteString(string svInput)
{
	if (!IsWritable())
	{
		return;
	}

	svInput += '\0'; // null-terminate the string.

	char* szText = const_cast<char*>(svInput.c_str());
	size_t nSize = svInput.size();

	writer.write(reinterpret_cast<const char*>(szText), nSize);
}
