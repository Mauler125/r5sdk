#pragma once

#include <memory>
#include <cstdint>
#include "Stream.h"
#include "IOError.h"
#include "TextWriter.h"

namespace IO
{
	class StreamWriter : public TextWriter
	{
	public:
		StreamWriter();
		StreamWriter(std::unique_ptr<Stream> Stream);
		StreamWriter(std::unique_ptr<Stream> Stream, bool LeaveOpen);
		StreamWriter(Stream* Stream);
		StreamWriter(Stream* Stream, bool LeaveOpen);
		virtual ~StreamWriter();

		// Implement functions
		virtual void Close();
		virtual void Flush();
		virtual void Write(const char Value);
		virtual void Write(const char* Buffer, uint32_t Index, uint32_t Count);

		// Writes a string to the file
		void Write(const string& Value);
		// Writes a string to the file and ends the line
		void WriteLine(const string& Value);

		// Writes a null-terminated string to the file
		void Write(const char* Value);
		// Writes a null-terminated string to the file and ends the line
		void WriteLine(const char* Value = nullptr);

		// Writes a formatted string to the file
		void WriteFmt(const char* Format, ...);
		// Writes a formatted string to the file and ends the line
		void WriteLineFmt(const char* Format, ...);

		// Get the underlying stream
		Stream* GetBaseStream() const;

	private:
		std::unique_ptr<Stream> BaseStream;
		bool _LeaveOpen;

		// Used for the built-in format buffer
		static uint32_t constexpr FormatBufferSize = 4096;
	};
}