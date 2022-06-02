#include "core/stdafx.h"
#include "public/include/binstream.h"

//-----------------------------------------------------------------------------
// Purpose: CIOStream constructors
//-----------------------------------------------------------------------------
CIOStream::CIOStream()
{
	m_eCurrentMode = Mode_t::NONE;
}
CIOStream::CIOStream(const string& svFileFullPath, Mode_t eMode)
{
	Open(svFileFullPath, eMode);
}

//-----------------------------------------------------------------------------
// Purpose: CIOStream destructor
//-----------------------------------------------------------------------------
CIOStream::~CIOStream()
{
	if (m_oStream.is_open())
		m_oStream.close();
	if (m_iStream.is_open())
		m_iStream.close();
}

//-----------------------------------------------------------------------------
// Purpose: opens the file in specified mode
// Input  : fileFullPath - mode
// Output : true if operation is successfull
//-----------------------------------------------------------------------------
bool CIOStream::Open(const string& svFilePath, Mode_t eMode)
{
	m_svFilePath = svFilePath;
	m_eCurrentMode = eMode;

	switch (m_eCurrentMode)
	{
	case Mode_t::READ:
		if (m_iStream.is_open())
		{
			m_iStream.close();
		}
		m_iStream.open(m_svFilePath.c_str(), std::ios::binary);
		if (!m_iStream.is_open())
		{
			Error(eDLL_T::FS, "Error opening file '%s' for read operation.\n", m_svFilePath.c_str());
			m_eCurrentMode = Mode_t::NONE;
		}
		if (!m_iStream.good())
		{
			Error(eDLL_T::FS, "Error opening file '%s' for read operation.\n", m_svFilePath.c_str());
			m_eCurrentMode = Mode_t::NONE;
			return false;
		}
		m_iStream.seekg(0, fstream::end);
		m_vData.resize(m_iStream.tellg());
		m_iStream.seekg(0, fstream::beg);
		m_iStream.read(reinterpret_cast<char*>(m_vData.data()), m_vData.size());
		m_iStream.seekg(0);
		m_iStream.clear();
		return true;

	case Mode_t::WRITE:
		if (m_oStream.is_open())
		{
			m_oStream.close();
		}
		m_oStream.open(m_svFilePath.c_str(), std::ios::binary);
		if (!m_oStream.is_open())
		{
			Error(eDLL_T::FS, "Error opening file '%s' for write operation.\n", m_svFilePath.c_str());
			m_eCurrentMode = Mode_t::NONE;
			return false;
		}
		if (!m_oStream.good())
		{
			Error(eDLL_T::FS, "Error opening file '%s' for write operation.\n", m_svFilePath.c_str());
			m_eCurrentMode = Mode_t::NONE;
			return false;
		}
		return true;

	default:
		m_eCurrentMode = Mode_t::NONE;
		return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: closes the stream
//-----------------------------------------------------------------------------
void CIOStream::Close()
{
	switch (m_eCurrentMode)
	{
	case Mode_t::READ:
		m_iStream.close();
		return;
	case Mode_t::WRITE:
		m_oStream.close();
		return;
	}
}

//-----------------------------------------------------------------------------
// Purpose: gets the possition of the current character in the stream
//-----------------------------------------------------------------------------
size_t CIOStream::GetPosition()
{
	switch (m_eCurrentMode)
	{
	case Mode_t::READ:
		return m_iStream.tellg();
		break;
	case Mode_t::WRITE:
		return m_oStream.tellp();
		break;
	default:
		return static_cast<size_t>(NULL);
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets the possition of the current character in the stream
// Input  : nOffset - 
//-----------------------------------------------------------------------------
void CIOStream::SetPosition(int64_t nOffset)
{
	switch (m_eCurrentMode)
	{
	case Mode_t::READ:
		m_iStream.seekg(nOffset);
		break;
	case Mode_t::WRITE:
		m_oStream.seekp(nOffset);
		break;
	default:
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns the vector (ifstream only)
//-----------------------------------------------------------------------------
const vector<uint8_t>& CIOStream::GetVector() const
{
	return m_vData;
}

//-----------------------------------------------------------------------------
// Purpose: returns the data (ifstream only)
//-----------------------------------------------------------------------------
const uint8_t* CIOStream::GetData() const
{
	return m_vData.data();
}

//-----------------------------------------------------------------------------
// Purpose: returns the data size (ifstream only)
//-----------------------------------------------------------------------------
const size_t CIOStream::GetSize() const
{
	return m_vData.size();
}

//-----------------------------------------------------------------------------
// Purpose: checks if we are able to read the file
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CIOStream::IsReadable()
{
	if (m_eCurrentMode != Mode_t::READ)
		return false;

	// check if we hit the end of the file.
	if (m_iStream.eof())
	{
		m_iStream.close();
		m_eCurrentMode = Mode_t::NONE;
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
	if (m_eCurrentMode != Mode_t::WRITE)
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: checks if we hit the end of file
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CIOStream::IsEof() const
{
	return m_iStream.eof();
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
		while (!m_iStream.eof() && (c = Read<char>()) != '\0')
			result += c;

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
		return;

	svInput += '\0'; // null-terminate the string.

	char* szText = const_cast<char*>(svInput.c_str());
	size_t nSize = svInput.size();

	m_oStream.write(reinterpret_cast<const char*>(szText), nSize);
}
