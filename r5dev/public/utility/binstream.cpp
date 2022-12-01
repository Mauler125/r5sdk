#include "core/stdafx.h"
#include "public/utility/binstream.h"

//-----------------------------------------------------------------------------
// Purpose: CIOStream constructors
//-----------------------------------------------------------------------------
CIOStream::CIOStream()
{
	m_eCurrentMode = Mode_t::NONE;
}
CIOStream::CIOStream(const fs::path& svFileFullPath, Mode_t eMode)
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
// Output : true if operation is successful
//-----------------------------------------------------------------------------
bool CIOStream::Open(const fs::path& fsFilePath, Mode_t eMode)
{
	m_eCurrentMode = eMode;

	switch (m_eCurrentMode)
	{
	case Mode_t::READ:
		if (m_iStream.is_open())
		{
			m_iStream.close();
		}
		m_iStream.open(fsFilePath, std::ios::binary | std::ios::in);
		if (!m_iStream.is_open() || !m_iStream.good())
		{
			m_eCurrentMode = Mode_t::NONE;
			return false;
		}

		ComputeFileSize();
		return true;

	case Mode_t::WRITE:
		if (m_oStream.is_open())
		{
			m_oStream.close();
		}
		m_oStream.open(fsFilePath, std::ios::binary | std::ios::out);
		if (!m_oStream.is_open() || !m_oStream.good())
		{
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
// Purpose: flushes the ofstream
//-----------------------------------------------------------------------------
void CIOStream::Flush()
{
	if (IsWritable())
		m_oStream.flush();
}

//-----------------------------------------------------------------------------
// Purpose: computes the input file size
//-----------------------------------------------------------------------------
void CIOStream::ComputeFileSize()
{
	m_nSize = m_iStream.tellg();
	m_iStream.seekg(0, std::ios::end);
	m_nSize = m_iStream.tellg() - m_nSize;
	m_iStream.seekg(0, std::ios::beg);
	m_iStream.clear();
}

//-----------------------------------------------------------------------------
// Purpose: gets the position of the current character in the stream
//-----------------------------------------------------------------------------
std::streampos CIOStream::GetPosition()
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
		return static_cast<std::streampos>(NULL);
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets the position of the current character in the stream
// Input  : nOffset - 
//-----------------------------------------------------------------------------
void CIOStream::SetPosition(std::streampos nOffset)
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
// Purpose: returns the data (ifstream only)
//-----------------------------------------------------------------------------
const std::filebuf* CIOStream::GetData() const
{
	return m_iStream.rdbuf();
}

//-----------------------------------------------------------------------------
// Purpose: returns the data size (ifstream only)
//-----------------------------------------------------------------------------
const std::streampos CIOStream::GetSize() const
{
	return m_nSize;
}

//-----------------------------------------------------------------------------
// Purpose: checks if we are able to read the file
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CIOStream::IsReadable()
{
	if (m_eCurrentMode != Mode_t::READ)
		return false;

	if (!m_iStream)
		return false;

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

	if (!m_oStream)
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
	string result;

	if (IsReadable())
	{
		char c;
		while (!m_iStream.eof() && (c = Read<char>()) != '\0')
			result += c;

		return result;
	}

	return result;
}

//-----------------------------------------------------------------------------
// Purpose: writes a string to the file
//-----------------------------------------------------------------------------
void CIOStream::WriteString(string svInput)
{
	if (!IsWritable())
		return;

	svInput += '\0'; // null-terminate the string.

	const char* szText = svInput.c_str();
	size_t nSize = svInput.size();

	m_oStream.write(szText, nSize);
}
