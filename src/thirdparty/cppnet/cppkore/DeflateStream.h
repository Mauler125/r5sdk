#pragma once

#include <cstdint>
#include <memory>
#include "Stream.h"
#include "CompressionMode.h"

namespace Compression
{
	// DeflateStream supports decompressing and compressing Deflate encoded data
	class DeflateStream : public IO::Stream
	{
	public:
		DeflateStream(std::unique_ptr<IO::Stream> Stream, CompressionMode Mode, bool LeaveOpen = false);
		DeflateStream(IO::Stream* Stream, CompressionMode Mode, bool LeaveOpen = false);
		virtual ~DeflateStream();

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
		virtual void Seek(uint64_t Offset, IO::SeekOrigin Origin);
		virtual uint64_t Read(uint8_t* Buffer, uint64_t Offset, uint64_t Count);
		virtual uint64_t Read(uint8_t* Buffer, uint64_t Offset, uint64_t Count, uint64_t Position);
		virtual void Write(uint8_t* Buffer, uint64_t Offset, uint64_t Count);
		virtual void Write(uint8_t* Buffer, uint64_t Offset, uint64_t Count, uint64_t Position);

	private:
		// Internal cached flags
		std::unique_ptr<IO::Stream> BaseStream;
		bool _LeaveOpen;

		// Internal state
		void* _DeflateState;

		// Internal buffer
		std::unique_ptr<uint8_t[]> _Buffer;
		uint32_t _BufferLength;
		uint32_t _BufferOffset;

		// Mode to use on the data
		CompressionMode _Mode;

		// An internal routine to setup the deflate state
		void CreateInflatorDeflator();
		// An internal routine to write deflater output
		void WriteDeflaterOutput();

		// The default buffer size for deflate streams
		constexpr static uint32_t DefaultBufferSize = 8192;
	};
}