#pragma once

#include <memory>
#include <cstdint>
#include "Stream.h"
#include "IOError.h"
#include "TextReader.h"

namespace IO
{
	class StreamReader : public TextReader
	{
	public:
		StreamReader();
		StreamReader(std::unique_ptr<Stream> Stream);
		StreamReader(std::unique_ptr<Stream> Stream, bool LeaveOpen);
		StreamReader(Stream* Stream);
		StreamReader(Stream* Stream, bool LeaveOpen);
		virtual ~StreamReader();

		// Implement functions
		virtual void Close();
		virtual int32_t Peek();
		virtual int32_t Read();

		// Get the underlying stream
		Stream* GetBaseStream() const;

	private:
		std::unique_ptr<Stream> BaseStream;
		std::unique_ptr<char[]> _InternalBuffer;
		bool _LeaveOpen;
		uint32_t _CharPos;
		uint32_t _CharLen;

		// Internal routine to read more data into the buffer
		int32_t ReadBuffer();

		// The default copy size for transfering buffers
		constexpr static uint64_t DefaultCharBufferSize = 4096;
	};
}