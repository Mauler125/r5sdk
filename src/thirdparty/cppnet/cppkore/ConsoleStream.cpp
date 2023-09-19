#include "stdafx.h"
#include "ConsoleStream.h"

namespace IO
{
	ConsoleStream::ConsoleStream(HANDLE StreamHandle, FileAccess Access)
		: _Handle(StreamHandle)
	{
		this->_CanRead = (((uint8_t)Access & (uint8_t)FileAccess::Read) == (uint8_t)FileAccess::Read);
		this->_CanRead = (((uint8_t)Access & (uint8_t)FileAccess::Write) == (uint8_t)FileAccess::Write);
		this->_IsPipe = (GetFileType(StreamHandle) == FILE_TYPE_PIPE);
	}

	ConsoleStream::~ConsoleStream()
	{
		this->Close();
	}

	void ConsoleStream::Seek(uint64_t Offset, SeekOrigin Origin)
	{
		IOError::StreamNoSeekSupport();
	}

	uint64_t ConsoleStream::Read(uint8_t* Buffer, uint64_t Offset, uint64_t Count)
	{
		// Wait for available input...
		ConsoleStream::WaitForAvailableConsoleInput(this->_Handle, this->_IsPipe);

		DWORD bRead = 0;
		ReadFile(this->_Handle, Buffer + Offset, (DWORD)Count, &bRead, NULL);

		return bRead;
	}

	uint64_t ConsoleStream::Read(uint8_t * Buffer, uint64_t Offset, uint64_t Count, uint64_t Position)
	{
		IOError::StreamNoSeekSupport();

		return 0;
	}

	void ConsoleStream::Write(uint8_t* Buffer, uint64_t Offset, uint64_t Count)
	{
		DWORD bWrite = 0;
		WriteFile(this->_Handle, Buffer + Offset, (DWORD)Count, &bWrite, NULL);
	}

	void ConsoleStream::Write(uint8_t * Buffer, uint64_t Offset, uint64_t Count, uint64_t Position)
	{
		IOError::StreamNoSeekSupport();
	}

	HANDLE ConsoleStream::GetStreamHandle() const
	{
		return this->_Handle;
	}

	void ConsoleStream::Close()
	{
		this->_Handle = nullptr;
		this->_CanRead = false;
		this->_CanWrite = false;
	}

	void ConsoleStream::Flush()
	{
	}

	bool ConsoleStream::CanRead()
	{
		return this->_CanRead;
	}

	bool ConsoleStream::CanWrite()
	{
		return this->_CanWrite;
	}

	bool ConsoleStream::CanSeek()
	{
		return false;
	}

	bool ConsoleStream::GetIsEndOfFile()
	{
		return false;
	}

	uint64_t ConsoleStream::GetLength()
	{
		return 0;
	}
	uint64_t ConsoleStream::GetPosition()
	{
		return 0;
	}

	void ConsoleStream::SetLength(uint64_t Length)
	{
		IOError::StreamNoSeekSupport();
	}

	void ConsoleStream::SetPosition(uint64_t Position)
	{
		IOError::StreamNoSeekSupport();
	}

	void ConsoleStream::WaitForAvailableConsoleInput(HANDLE hHandle, bool IsPipe)
	{
		bool sWait = false;

		if (IsPipe)
		{

			DWORD cBytesRead, cTotalBytesAvailable, cBytesLeftThisMessage;
			auto Result = PeekNamedPipe(hHandle, NULL, 0, &cBytesRead, &cTotalBytesAvailable, &cBytesLeftThisMessage);
			if (Result != 0)
			{
				sWait = (cTotalBytesAvailable > 0);
			}
			else
			{
				auto eCode = GetLastError();
				sWait = eCode == ERROR_BROKEN_PIPE || eCode == ERROR_NO_DATA || eCode == ERROR_PIPE_NOT_CONNECTED;
			}
		}

		if (!sWait)
			WaitForSingleObjectEx(hHandle, INFINITE, TRUE);
	}
}
