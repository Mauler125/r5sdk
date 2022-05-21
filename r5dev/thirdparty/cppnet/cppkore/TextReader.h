#pragma once

#include <cstdint>
#include "IOError.h"
#include "StringBase.h"

namespace IO
{
	class TextReader
	{
	public:
		// Abstract Dtor
		virtual ~TextReader() = default;

		// Abstract functions
		virtual void Close() = 0;
		virtual int32_t Peek() = 0;
		virtual int32_t Read() = 0;
		// Reads a block of characters. This method will read up to
		// count characters from this TextReader.
		virtual int32_t Read(char* Buffer, uint32_t Index, uint32_t Count);
		// Reads all characters from the current position to the end of the 
		// TextReader, and returns them as one string.
		virtual string ReadToEnd();
		// Reads a line. A line is defined as a sequence of characters followed by
		// a carriage return ('\r'), a line feed ('\n'), or a carriage return
		// immediately followed by a line feed.
		virtual string ReadLine();
	};
}