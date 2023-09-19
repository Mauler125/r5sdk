#pragma once

#include <cstdint>
#include "SeekOrigin.h"
#include "IOError.h"

namespace IO
{
	class Stream
	{
	public:
		// Abstract Dtor
		virtual ~Stream() = default;

		// Abstract Getters and Setters
		virtual bool CanRead() = 0;
		virtual bool CanWrite() = 0;
		virtual bool CanSeek() = 0;
		virtual bool GetIsEndOfFile() = 0;
		virtual uint64_t GetLength() = 0;
		virtual uint64_t GetPosition() = 0;
		virtual void SetLength(uint64_t Length) = 0;
		virtual void SetPosition(uint64_t Position) = 0;

		// Abstract functions
		virtual void Close() = 0;
		virtual void Flush() = 0;
		virtual void Seek(uint64_t Offset, SeekOrigin Origin) = 0;
		virtual uint64_t Read(uint8_t* Buffer, uint64_t Offset, uint64_t Count) = 0;
		virtual uint64_t Read(uint8_t* Buffer, uint64_t Offset, uint64_t Count, uint64_t Position) = 0;
		virtual void Write(uint8_t* Buffer, uint64_t Offset, uint64_t Count) = 0;
		virtual void Write(uint8_t* Buffer, uint64_t Offset, uint64_t Count, uint64_t Position) = 0;

		// Copies all of the data from the current position to the target stream
		void CopyTo(Stream& Rhs)
		{
			if (!this->CanRead())
				IOError::StreamNoReadSupport();
			if (!Rhs.CanWrite())
				IOError::StreamNoWriteSupport();
			
			auto Buffer = std::make_unique<uint8_t[]>(DefaultCopyBufferSize);
			uint64_t Read = 0;
			while ((Read = this->Read(Buffer.get(), 0, DefaultCopyBufferSize)) != 0)
				Rhs.Write(Buffer.get(), 0, Read);
		}

		// Copies all of the data from the current position to the target stream
		void CopyTo(std::unique_ptr<Stream>& Rhs)
		{
			this->CopyTo(Rhs.get());
		}

		// Copies all of the data from the current position to the target stream
		void CopyTo(Stream* Rhs)
		{
			if (!this->CanRead())
				IOError::StreamNoReadSupport();
			if (!Rhs->CanWrite())
				IOError::StreamNoWriteSupport();

			auto Buffer = std::make_unique<uint8_t[]>(DefaultCopyBufferSize);
			uint64_t Read = 0;
			while ((Read = this->Read(Buffer.get(), 0, DefaultCopyBufferSize)) != 0)
				Rhs->Write(Buffer.get(), 0, Read);
		}

	private:

		// The default copy size for transfering buffers
		constexpr static uint64_t DefaultCopyBufferSize = 81920;
	};
}