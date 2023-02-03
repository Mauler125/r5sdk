#include "core/stdafx.h"
#include "public/utility/binstream.h"

//-----------------------------------------------------------------------------
// Purpose: CIOStream constructors
//-----------------------------------------------------------------------------
CIOStream::CIOStream()
{
	m_nSize = 0;
	m_nFlags = Mode_t::NONE;
}
CIOStream::CIOStream(const fs::path& svFileFullPath, int nFlags)
{
	Open(svFileFullPath, nFlags);
}

//-----------------------------------------------------------------------------
// Purpose: CIOStream destructor
//-----------------------------------------------------------------------------
CIOStream::~CIOStream()
{
	if (m_Stream.is_open())
		m_Stream.close();
	if (m_Stream.is_open())
		m_Stream.close();
}

//-----------------------------------------------------------------------------
// Purpose: opens the file in specified mode
// Input  : &fsFilePath - 
//			nFlags - 
// Output : true if operation is successful
//-----------------------------------------------------------------------------
bool CIOStream::Open(const fs::path& fsFilePath, int nFlags)
{
	m_nFlags = nFlags;

	if (m_Stream.is_open())
	{
		m_Stream.close();
	}
	m_Stream.open(fsFilePath, nFlags);
	if (!m_Stream.is_open() || !m_Stream.good())
	{
		m_nFlags = Mode_t::NONE;
		return false;
	}

	if (nFlags & Mode_t::READ)
	{
		ComputeFileSize();
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: closes the stream
//-----------------------------------------------------------------------------
void CIOStream::Close()
{
	m_Stream.close();
}

//-----------------------------------------------------------------------------
// Purpose: flushes the ofstream
//-----------------------------------------------------------------------------
void CIOStream::Flush()
{
	if (IsWritable())
		m_Stream.flush();
}

//-----------------------------------------------------------------------------
// Purpose: computes the input file size
//-----------------------------------------------------------------------------
void CIOStream::ComputeFileSize()
{
	m_nSize = m_Stream.tellg();
	m_Stream.seekg(0, std::ios::end);
	m_nSize = m_Stream.tellg() - m_nSize;
	m_Stream.seekg(0, std::ios::beg);
	m_Stream.clear();
}

//-----------------------------------------------------------------------------
// Purpose: gets the position of the current character in the stream
// Input  : mode - 
// Output : std::streampos
//-----------------------------------------------------------------------------
std::streampos CIOStream::GetPosition(Mode_t mode)
{
	switch (mode)
	{
	case Mode_t::READ:
		return m_Stream.tellg();
		break;
	case Mode_t::WRITE:
		return m_Stream.tellp();
		break;
	default:
		return static_cast<std::streampos>(NULL);
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets the position of the current character in the stream
// Input  : nOffset - 
//			mode - 
//-----------------------------------------------------------------------------
void CIOStream::SetPosition(std::streampos nOffset, Mode_t mode)
{
	switch (mode)
	{
	case Mode_t::READ:
		m_Stream.seekg(nOffset);
		break;
	case Mode_t::WRITE:
		m_Stream.seekp(nOffset);
		break;
	default:
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns the data (ifstream only)
// Output : std::filebuf*
//-----------------------------------------------------------------------------
const std::filebuf* CIOStream::GetData() const
{
	return m_Stream.rdbuf();
}

//-----------------------------------------------------------------------------
// Purpose: returns the data size (ifstream only)
// Output : std::streampos
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
	if (!(m_nFlags & Mode_t::READ) || !m_Stream || m_Stream.eof())
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: checks if we are able to write to file
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CIOStream::IsWritable() const
{
	if (!(m_nFlags & Mode_t::WRITE) || !m_Stream)
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: checks if we hit the end of file
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CIOStream::IsEof() const
{
	return m_Stream.eof();
}

//-----------------------------------------------------------------------------
// Purpose: reads a string from the file and returns it
// Input  : &svOut - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CIOStream::ReadString(string& svOut)
{
	if (IsReadable())
	{
		char c;
		while (!m_Stream.eof() && (c = Read<char>()) != '\0')
			svOut += c;

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: writes a string to the file
// Input  : &svInput - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CIOStream::WriteString(const string& svInput)
{
	if (!IsWritable())
		return false;

	const char* szText = svInput.c_str();
	size_t nSize = svInput.size();

	m_Stream.write(szText, nSize);
	m_nSize += nSize;

	return true;
}
