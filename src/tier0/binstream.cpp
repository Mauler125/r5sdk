#include "tier0/binstream.h"
#include <sys/stat.h>

//-----------------------------------------------------------------------------
// Purpose: CIOStream constructors
//-----------------------------------------------------------------------------
CIOStream::CIOStream()
{
	m_nSize = 0;
	m_nFlags = Mode_t::NONE;
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
// Input  : *pFilePath - 
//			nFlags - 
// Output : true if operation is successful
//-----------------------------------------------------------------------------
bool CIOStream::Open(const char* const pFilePath, const int nFlags)
{
	m_nFlags = nFlags;

	if (m_Stream.is_open())
	{
		m_Stream.close();
	}
	m_Stream.open(pFilePath, nFlags);
	if (!m_Stream.is_open() || !m_Stream.good())
	{
		m_nFlags = Mode_t::NONE;
		return false;
	}

	if (nFlags & Mode_t::READ)
	{
		struct _stat64 status;
		if (_stat64(pFilePath, &status) != NULL)
		{
			return false;
		}

		m_nSize = status.st_size;
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
// Purpose: gets the position of the current character in the stream
// Input  : mode - 
// Output : std::streampos
//-----------------------------------------------------------------------------
std::streampos CIOStream::TellGet()
{
	return m_Stream.tellg();
}
std::streampos CIOStream::TellPut()
{
	return m_Stream.tellp();
}

//-----------------------------------------------------------------------------
// Purpose: sets the position of the current character in the stream
// Input  : nOffset - 
//			mode - 
//-----------------------------------------------------------------------------
void CIOStream::SeekGet(const std::streampos nOffset)
{
	m_Stream.seekg(nOffset, std::ios::beg);
}
void CIOStream::SeekPut(const std::streampos nOffset)
{
	m_Stream.seekp(nOffset, std::ios::beg);
}
void CIOStream::Seek(const std::streampos nOffset)
{
	SeekGet(nOffset);
	SeekPut(nOffset);
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
// Purpose: reads a string from the file
// Input  : &svOut - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CIOStream::ReadString(std::string& svOut)
{
	if (IsReadable())
	{
		while (!m_Stream.eof())
		{
			const char c = Read<char>();

			if (c == '\0')
				break;

			svOut += c;
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: reads a string from the file into a fixed size buffer
// Input  : *pBuf - 
//			nLen - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CIOStream::ReadString(char* const pBuf, const size_t nLen)
{
	if (IsReadable())
	{
		size_t i = 0;

		while (i < nLen && !m_Stream.eof())
		{
			const char c = Read<char>();

			if (c == '\0')
				break;

			pBuf[i++] = c;
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: writes a string to the file
// Input  : &svInput - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CIOStream::WriteString(const std::string& svInput)
{
	if (!IsWritable())
		return false;

	const char* const szText = svInput.c_str();
	const size_t nSize = svInput.size();

	m_Stream.write(szText, nSize);
	m_nSize += nSize;

	return true;
}
