#include "stdafx.h"
#include "FileStream.h"

namespace IO
{
	FileStream::FileStream(const string& Path, FileMode Mode)
		: FileStream(Path, Mode, (Mode == FileMode::Append ? FileAccess::Write : FileAccess::ReadWrite), FileShare::Read)
	{
	}

	FileStream::FileStream(const string& Path, FileMode Mode, FileAccess Access)
		: FileStream(Path, Mode, Access, FileShare::Read)
	{
	}

	FileStream::FileStream(const string& Path, FileMode Mode, FileAccess Access, FileShare Share)
		: FileStream(Path, Mode, Access, Share, FileStream::DefaultBufferSize)
	{
	}

	FileStream::FileStream(const string & Path, FileMode Mode, FileAccess Access, FileShare Share, uint32_t BufferSize)
	{
		this->SetupStream(Path, Mode, Access, Share, BufferSize);
	}

	FileStream::~FileStream()
	{
		this->Close();
	}

	bool FileStream::CanRead()
	{
		return _CanRead;
	}

	bool FileStream::CanWrite()
	{
		return _CanWrite;
	}

	bool FileStream::CanSeek()
	{
		return _CanSeek;
	}

	bool FileStream::GetIsEndOfFile()
	{
		if (_CanRead)
		{
			if (this->GetLength() == this->GetPosition())
				return true;
			else
				return false;
		}

		return false;
	}

	uint64_t FileStream::GetLength()
	{
		if (!this->_Handle)
			IOError::StreamNotOpen();

		int64_t Result = 0;
		GetFileSizeEx(this->_Handle, (PLARGE_INTEGER)&Result);

		if (this->_WritePosition > 0 && (this->_Position + this->_WritePosition) > Result)
			Result = (this->_WritePosition + this->_Position);

		return Result;
	}

	uint64_t FileStream::GetPosition()
	{
		if (!this->_Handle)
			IOError::StreamNotOpen();

		VerifyOSHandlePosition();

		return (uint64_t)(this->_Position + (this->_ReadPosition - this->_ReadLength + this->_WritePosition));
	}

	void FileStream::SetLength(uint64_t Length)
	{
		IOError::StreamSetLengthSupport();
	}

	void FileStream::SetPosition(uint64_t Position)
	{
		if (!this->_Handle)
			IOError::StreamNotOpen();

		if (!_CanSeek)
			IOError::StreamNoSeekSupport();

		if (this->_WritePosition > 0)
			this->FlushWrite();
		this->_ReadPosition = 0;
		this->_ReadLength = 0;

		LARGE_INTEGER Move{};
		Move.QuadPart = Position;

		SetFilePointerEx(this->_Handle, Move, NULL, FILE_BEGIN);
	}

	void FileStream::Close()
	{
		if (this->_WritePosition > 0)
			this->FlushWrite();

		if (this->_Handle)
			CloseHandle(this->_Handle);

		this->_Handle = nullptr;
		this->_CanRead = false;
		this->_CanSeek = false;
		this->_CanWrite = false;

		this->_Buffer.reset();
	}

	void FileStream::Flush()
	{
		if (!this->_Handle)
			IOError::StreamNotOpen();

		if (this->_WritePosition > 0)
			this->FlushWrite();
		if (this->_ReadPosition < this->_ReadLength)
			this->FlushRead();
	}

	void FileStream::Seek(uint64_t Offset, SeekOrigin Origin)
	{
		if (!this->_Handle)
			IOError::StreamNotOpen();

		if (!_CanSeek)
			IOError::StreamNoSeekSupport();

		// Handle simulated seeking when our buffer already covers the segment
		if (this->_WritePosition > 0)
			this->FlushWrite();
		else if (Origin == SeekOrigin::Current)
			Offset -= (this->_ReadLength - this->_ReadPosition);

		// Validate that our internal position matches the handle position
		VerifyOSHandlePosition();
		
		auto OldPosition = this->_Position + (this->_ReadPosition - this->_ReadLength);

		LARGE_INTEGER Distance{};
		LARGE_INTEGER Position{};
		Distance.QuadPart = Offset;

		// Perform the seek
		switch (Origin)
		{
		case SeekOrigin::Begin:
			SetFilePointerEx(this->_Handle, Distance, &Position, FILE_BEGIN);
			break;
		case SeekOrigin::Current:
			SetFilePointerEx(this->_Handle, Distance, &Position, FILE_CURRENT);
			break;
		case SeekOrigin::End:
			SetFilePointerEx(this->_Handle, Distance, &Position, FILE_END);
			break;
		}
		this->_Position = Position.QuadPart;

		// Emulated seek in our buffer
		if (this->_ReadLength > 0)
		{
			if (OldPosition == Position.QuadPart)
			{
				if (this->_ReadPosition > 0)
				{
					std::memcpy(this->_Buffer.get(), this->_Buffer.get() + this->_ReadPosition, (this->_ReadLength - this->_ReadPosition));
					this->_ReadLength -= this->_ReadPosition;
					this->_ReadPosition = 0;
				}

				if (this->_ReadLength > 0)
				{
					Distance.QuadPart = this->_ReadLength;
					SetFilePointerEx(this->_Handle, Distance, &Position, FILE_CURRENT);
					this->_Position = Position.QuadPart;
				}
			}
			else if (OldPosition - this->_ReadPosition < this->_Position && this->_Position < OldPosition + this->_ReadLength - this->_ReadPosition)
			{
				auto Difference = (this->_Position - OldPosition);
				std::memcpy(this->_Buffer.get(), this->_Buffer.get() + (this->_ReadPosition + Difference), this->_ReadLength - (this->_ReadPosition + Difference));

				this->_ReadLength -= (uint32_t)(this->_ReadPosition + Difference);
				this->_ReadPosition = 0;

				if (this->_ReadLength > 0)
				{
					Distance.QuadPart = this->_ReadLength;
					SetFilePointerEx(this->_Handle, Distance, &Position, FILE_CURRENT);
					this->_Position = Position.QuadPart;
				}
			}
			else
			{
				this->_ReadPosition = 0;
				this->_ReadLength = 0;
			}
		}
	}

	uint64_t FileStream::Read(uint8_t* Buffer, uint64_t Offset, uint64_t Count)
	{
		if (!this->_Handle)
			IOError::StreamNotOpen();

		if (!this->_CanRead)
			IOError::StreamNoReadSupport();

		bool isBlocked = false;
		uint64_t BufferRemaining = this->_ReadLength - this->_ReadPosition;
		if (BufferRemaining == 0)
		{
			if (this->_WritePosition > 0)
				this->FlushWrite();

			if (!this->_CanSeek || (Count >= this->_BufferSize))
			{
				auto ResultRead = this->ReadCore(Buffer, Offset, Count);

				this->_ReadPosition = 0;
				this->_ReadLength = 0;

				return ResultRead;
			}

			BufferRemaining = this->ReadCore(this->_Buffer.get(), 0, this->_BufferSize);

			if (BufferRemaining == 0)
				return 0;
			isBlocked = (BufferRemaining < this->_BufferSize);

			this->_ReadPosition = 0;
			this->_ReadLength = (uint32_t)BufferRemaining;
		}

		if (BufferRemaining > Count)
			BufferRemaining = Count;

		std::memcpy(Buffer + Offset, this->_Buffer.get() + this->_ReadPosition, BufferRemaining);
		this->_ReadPosition += (uint32_t)BufferRemaining;

		if (BufferRemaining < Count && !isBlocked)
		{
			auto MoreFill = this->ReadCore(Buffer, Offset + BufferRemaining, Count - BufferRemaining);

			BufferRemaining += MoreFill;

			this->_ReadPosition = 0;
			this->_ReadLength = 0;
		}

		return BufferRemaining;
	}

	uint64_t FileStream::Read(uint8_t* Buffer, uint64_t Offset, uint64_t Count, uint64_t Position)
	{
		this->SetPosition(Position);
		return this->Read(Buffer, Offset, Count);
	}

	void FileStream::Write(uint8_t* Buffer, uint64_t Offset, uint64_t Count)
	{
		if (!this->_Handle)
			IOError::StreamNotOpen();

		if (!this->_CanWrite)
			IOError::StreamNoWriteSupport();

		if (this->_WritePosition == 0)
		{
			if (this->_ReadPosition < this->_ReadLength)
				this->FlushRead();

			this->_ReadPosition = 0;
			this->_ReadLength = 0;
		}

		if (this->_WritePosition > 0)
		{
			auto BufferLeft = (this->_BufferSize - this->_WritePosition);
			if (BufferLeft > 0)
			{
				if (BufferLeft > Count)
					BufferLeft = (uint32_t)Count;

				std::memcpy(this->_Buffer.get() + this->_WritePosition, Buffer + Offset, BufferLeft);

				this->_WritePosition += BufferLeft;

				if (Count == BufferLeft)
					return;

				Offset += BufferLeft;
				Count -= BufferLeft;
			}

			// Flush the write buffer
			this->WriteCore(this->_Buffer.get(), 0, this->_WritePosition);
			this->_WritePosition = 0;
		}

		if (Count >= this->_BufferSize)
		{
			this->WriteCore(Buffer, Offset, Count);
			return;
		}

		std::memcpy(this->_Buffer.get() + this->_WritePosition, Buffer + Offset, Count);
		this->_WritePosition = (uint32_t)Count;
	}

	void FileStream::Write(uint8_t* Buffer, uint64_t Offset, uint64_t Count, uint64_t Position)
	{
		this->SetPosition(Position);
		this->Write(Buffer, Offset, Count);
	}

	void FileStream::FlushWrite()
	{
		this->WriteCore(this->_Buffer.get(), 0, this->_WritePosition);
		this->_WritePosition = 0;
	}

	void FileStream::FlushRead()
	{
		if ((this->_ReadPosition - this->_ReadLength) != 0)
		{
			LARGE_INTEGER Distance{};
			Distance.QuadPart = (this->_ReadPosition - this->_ReadLength);

			LARGE_INTEGER Position{};
			SetFilePointerEx(this->_Handle, Distance, &Position, FILE_CURRENT);

			this->_Position = Position.QuadPart;
		}

		this->_ReadPosition = 0;
		this->_ReadLength = 0;
	}

	void FileStream::VerifyOSHandlePosition()
	{
		LARGE_INTEGER Distance{};
		LARGE_INTEGER Result{};

		// Save the old position, and grab the file position
		auto OldPosition = this->_Position;
		SetFilePointerEx(this->_Handle, Distance, &Result, FILE_CURRENT);
		this->_Position = Result.QuadPart;

		// Check for out of sync file position
		if (Result.QuadPart != OldPosition)
		{
			this->_ReadPosition = 0;
			this->_ReadLength = 0;
		}
	}

	void FileStream::WriteCore(uint8_t* Buffer, uint64_t Offset, uint64_t Count)
	{
		auto WritePtr = (Buffer + Offset);
		uint64_t TotalWrite = 0;

		VerifyOSHandlePosition();

		DWORD nWrite = 0;
		while (Count > 0 && this->_Handle)
		{
			// Calculate based on DWORD MAX value due to API limitations
			auto Want = (Count > UINT32_MAX) ? UINT32_MAX : Count;

			// Write the buffer
			WriteFile(this->_Handle, WritePtr, (DWORD)Want, &nWrite, NULL);

			// Adjust counts
			TotalWrite += nWrite;
			Count -= Want;
			WritePtr += Want;
		}

		this->_Position += TotalWrite;
	}

	uint64_t FileStream::ReadCore(uint8_t* Buffer, uint64_t Offset, uint64_t Count)
	{
		auto ReadPtr = (Buffer + Offset);
		uint64_t TotalRead = 0;

		VerifyOSHandlePosition();

		DWORD nRead = 0;
		while (Count > 0 && this->_Handle)
		{
			// Calculate based on DWORD MAX value due to API limitations
			auto Want = (Count > UINT32_MAX) ? UINT32_MAX : Count;

			// Read the buffer
			ReadFile(this->_Handle, ReadPtr, (DWORD)Want, &nRead, NULL);

			// Adjust counts
			TotalRead += nRead;
			Count -= Want;
			ReadPtr += Want;
		}

		this->_Position += TotalRead;

		return TotalRead;
	}

	void FileStream::SetupStream(const string& Path, FileMode Mode, FileAccess Access, FileShare Share, uint32_t BufferSize)
	{
		// Easy way to make sure we have a fresh stream
		this->Close();

		// Prepare CreateFileA flags against our options
		auto fAccess = GENERIC_READ;
		switch (Access)
		{
		case FileAccess::Write:
			fAccess = GENERIC_WRITE;
			break;
		case FileAccess::ReadWrite:
			fAccess |= GENERIC_WRITE;
			break;
		}

		auto fShare = NULL;
		switch (Share)
		{
		case FileShare::Read:
			fShare = FILE_SHARE_READ;
			break;
		case FileShare::Write:
			fShare = FILE_SHARE_WRITE;
			break;
		case FileShare::ReadWrite:
			fShare = (FILE_SHARE_READ | FILE_SHARE_WRITE);
			break;
		case FileShare::Delete:
			fShare = FILE_SHARE_DELETE;
			break;
		}

		auto fMode = CREATE_NEW;
		switch (Mode)
		{
		case FileMode::Create:
			fMode = CREATE_ALWAYS;
			break;
		case FileMode::OpenOrCreate:
			fMode = OPEN_ALWAYS;
			break;
		case FileMode::Open:
			fMode = OPEN_EXISTING;
			break;
		case FileMode::Truncate:
			fMode = TRUNCATE_EXISTING;
			break;
		case FileMode::Append:
			fMode = OPEN_EXISTING;
			break;
		}

		// Open a native file handle
		auto hFile = CreateFileA(Path.begin(), fAccess, fShare, NULL, fMode, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			switch (GetLastError())
			{
			case ERROR_FILE_EXISTS:
				IOError::StreamFileExists();
				break;
			case ERROR_PATH_NOT_FOUND:
				IOError::StreamPathInvalid();
				break;
			case ERROR_FILE_NOT_FOUND:
				IOError::StreamFileNotFound();
				break;
			case ERROR_SHARING_VIOLATION:
				IOError::StreamInUse();
				break;
			default:
				IOError::StreamUnknown();
				break;
			}
		}

		// Set the handle
		this->_Handle = hFile;

		// Set flags once we are sure it's open
		switch (Access)
		{
		case FileAccess::Read:
			this->_CanRead = true;
			break;
		case FileAccess::Write:
			this->_CanWrite = true;
			break;
		case FileAccess::ReadWrite:
			this->_CanRead = true;
			this->_CanWrite = true;
			break;
		}

		// We can seek if we aren't in append mode, otherwise, move to end for appending...
		LARGE_INTEGER Position{};
		if (Mode != FileMode::Append)
			this->_CanSeek = true;
		else
		{
			LARGE_INTEGER Move{};
			SetFilePointerEx(this->_Handle, Move, &Position, FILE_END);
		}

		// Setup the buffer
		this->_Buffer = std::make_unique<uint8_t[]>(BufferSize);
		this->_BufferSize = BufferSize;

		// Initialize structures
		this->_ReadLength = 0;
		this->_ReadPosition = 0;
		this->_WritePosition = 0;
		this->_Position = Position.QuadPart;
	}
}