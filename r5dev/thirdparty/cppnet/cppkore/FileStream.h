#pragma once

#include <cstdint>
#include <memory>

#include "Stream.h"
#include "StringBase.h"
#include "FileMode.h"
#include "FileAccess.h"
#include "FileShare.h"

namespace IO
{
	// FileStream supports reading and writing files to the disk
	class FileStream : public Stream
	{
	public:
		FileStream(const string& Path, FileMode Mode);
		FileStream(const string& Path, FileMode Mode, FileAccess Access);
		FileStream(const string& Path, FileMode Mode, FileAccess Access, FileShare Share);
		FileStream(const string& Path, FileMode Mode, FileAccess Access, FileShare Share, uint32_t BufferSize);
		virtual ~FileStream();

		// Implement Getters and Setters
		virtual bool CanRead();
		virtual bool CanWrite();
		virtual bool CanSeek();
		virtual bool GetIsEndOfFile();
		virtual uint64_t GetLength();
		virtual uint64_t GetPosition();
		virtual void SetLength(uint64_t Length);
		virtual void SetPosition(uint64_t Position);

		// Implement functions
		virtual void Close();
		virtual void Flush();
		virtual void Seek(uint64_t Offset, SeekOrigin Origin);
		virtual uint64_t Read(uint8_t* Buffer, uint64_t Offset, uint64_t Count);
		virtual uint64_t Read(uint8_t* Buffer, uint64_t Offset, uint64_t Count, uint64_t Position);
		virtual void Write(uint8_t* Buffer, uint64_t Offset, uint64_t Count);
		virtual void Write(uint8_t* Buffer, uint64_t Offset, uint64_t Count, uint64_t Position);

	private:
		// FileMode flags cached
		bool _CanRead;
		bool _CanWrite;
		bool _CanSeek;

		// Internal buffer
		std::unique_ptr<uint8_t[]> _Buffer;
		uint32_t _BufferSize;

		// Internal cached positions
		int32_t _ReadLength;
		int32_t _ReadPosition;
		int32_t _WritePosition;
		int64_t _Position;

		// The file handle
		HANDLE _Handle;

		// Internal routines to flush the buffers
		void FlushWrite();
		void FlushRead();

		// Internal routine to verify the handle position
		void VerifyOSHandlePosition();

		// Internal routines for reading and writing
		void WriteCore(uint8_t* Buffer, uint64_t Offset, uint64_t Count);
		uint64_t ReadCore(uint8_t* Buffer, uint64_t Offset, uint64_t Count);

		// Sets up the FileStream
		void SetupStream(const string& Path, FileMode Mode, FileAccess Access, FileShare Share, uint32_t BufferSize);

		// Internal buffer size default 4k
		constexpr static uint32_t DefaultBufferSize = 4096;
	};
}