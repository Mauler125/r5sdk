#include "core/stdafx.h"
#include "engine/sys_utils.h"
#include "public/include/binstream.h"

//-----------------------------------------------------------------------------
// Purpose: CIOStream constructor
//-----------------------------------------------------------------------------
CIOStream::CIOStream()
{
	eCurrentMode = eStreamFileMode::NONE;
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
bool CIOStream::open(std::string svFileFullPath, eStreamFileMode eMode)
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
			DevMsg(eDLL_T::FS, "Error opening file '%s' for write operation!\n", svFilePath.c_str());
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
			DevMsg(eDLL_T::FS, "Error opening file '%s' for read operation!\n", svFilePath.c_str());
			eCurrentMode = eStreamFileMode::NONE;
		}
	}

	// if the mode is still the NONE -> we failed
	return eCurrentMode == eStreamFileMode::NONE ? false : true;
}

//-----------------------------------------------------------------------------
// Purpose: closes the file
//-----------------------------------------------------------------------------
void CIOStream::close()
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
// Purpose: checks if we are able to read the file
//-----------------------------------------------------------------------------
bool CIOStream::checkReadabilityStatus()
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
//-----------------------------------------------------------------------------
bool CIOStream::checkWritabilityStatus()
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
//-----------------------------------------------------------------------------
bool CIOStream::eof()
{
	return reader.eof();
}

//-----------------------------------------------------------------------------
// Purpose: reads a string from the file and returns it
//-----------------------------------------------------------------------------
std::string CIOStream::readString()
{
	if (checkReadabilityStatus())
	{
		char c;
		std::string result = "";
		while (!reader.eof() && (c = readR<char>()) != '\0')
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
void CIOStream::writeString(std::string str)
{
	if (!checkWritabilityStatus())
	{
		return;
	}

	str += '\0'; // null-terminate the string.

	char* text = (char*)(str.c_str());
	size_t size = str.size();

	writer.write((const char*)text, size);
}
