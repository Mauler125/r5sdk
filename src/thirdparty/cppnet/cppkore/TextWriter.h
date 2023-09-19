#pragma once

#include <cstdint>
#include "IOError.h"
#include "StringBase.h"

namespace IO
{
	class TextWriter
	{
	public:
		// Abstract Dtor
		virtual ~TextWriter() = default;

		// Abstract functions
		virtual void Close() = 0;
		virtual void Flush() = 0;
		virtual void Write(const char Value) = 0;
		virtual void Write(const char* Buffer, uint32_t Index, uint32_t Count) = 0;
		// Ends the current line
		virtual void WriteLine();
		// Writes a character then ends the line
		virtual void WriteLine(const char Value);
		// Writes a character array to the stream then ends the line
		virtual void WriteLine(const char* Buffer, uint32_t Index, uint32_t Count);

	private:

		// The internal newline sequence
		constexpr static char NewLine[] = { '\r', '\n' };
	};
}