#pragma once

#include <memory>
#include "Stream.h"

namespace IO
{
	// MemoryStream supports reading and writing data to memory
	class MemoryStream : public Stream
	{
	public:
		MemoryStream();
		MemoryStream(uint64_t Capacity);
		MemoryStream(uint8_t* Buffer, uint64_t Index, uint64_t Count);
		MemoryStream(uint8_t* Buffer, uint64_t Index, uint64_t Count, bool Writable);
		MemoryStream(uint8_t* Buffer, uint64_t Index, uint64_t Count, bool Writable, bool LeaveOpen);
		MemoryStream(uint8_t* Buffer, uint64_t Index, uint64_t Count, bool Writable, bool LeaveOpen, bool Expandable);
		virtual ~MemoryStream();

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
		// Memory flags cached
		bool _CanWrite;
		bool _Expandable;
		bool _KeepOpen;
		uint64_t _Origin;
		uint64_t _Length;
		uint64_t _Position;

		// Memory buffer and size, internal
		uint64_t _BufferSize;
		uint8_t* _Buffer;

		// Handles reallocation of the internal buffer
		void EnsureCapacity(uint64_t Size);
	};
}