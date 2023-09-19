#pragma once

#include <cstdint>
#include <memory>
#include "Stream.h"
#include "CompressionMode.h"

namespace Compression
{
	// LZ4Stream supports decompressing and compressing LZ4 encoded data
	class LZ4Stream : public IO::Stream
	{
	public:
		LZ4Stream(std::unique_ptr<IO::Stream> Stream, CompressionMode Mode, bool HighCompression = false, uint32_t BlockSize = (1024 * 1024), bool LeaveOpen = false);
		LZ4Stream(IO::Stream* Stream, CompressionMode Mode, bool HighCompression = false, uint32_t BlockSize = (1024 * 1024), bool LeaveOpen = false);
		virtual ~LZ4Stream();

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
		bool _HighCompression;

		// The maximum size of each block
		uint32_t _BlockSize;

		// Internal compression buffer
		std::unique_ptr<uint8_t[]> _Buffer;
		uint32_t _BufferLength;
		uint32_t _BufferOffset;

		// Mode to use on the data
		CompressionMode _Mode;

		// Internal routine to load the next chunk
		bool AcquireNextChunk();
		// Internal routine to parse a varint from the stream
		bool TryReadVarInt(uint64_t& Result);
		// Internal routine to parse a varint
		uint64_t ReadVarInt();
		// Internal routine to compress the current chunk
		void FlushCurrentChunk();
		// Internal routine to write a varint
		void WriteVarInt(uint64_t Value);
	};
}