#include "stdafx.h"
#include "StreamWriter.h"

namespace IO
{
	StreamWriter::StreamWriter()
		: StreamWriter(nullptr, false)
	{
	}

	StreamWriter::StreamWriter(std::unique_ptr<Stream> Stream)
		: StreamWriter(std::move(Stream), false)
	{
	}

	StreamWriter::StreamWriter(std::unique_ptr<Stream> Stream, bool LeaveOpen)
	{
		this->BaseStream = std::move(Stream);
		this->_LeaveOpen = LeaveOpen;
	}

	StreamWriter::StreamWriter(Stream* Stream)
		: StreamWriter(Stream, false)
	{
	}

	StreamWriter::StreamWriter(Stream* Stream, bool LeaveOpen)
	{
		this->BaseStream.reset(Stream);
		this->_LeaveOpen = LeaveOpen;
	}

	StreamWriter::~StreamWriter()
	{
		this->Close();
	}

	void StreamWriter::Close()
	{
		// Forcefully reset the stream
		if (this->_LeaveOpen)
			this->BaseStream.release();
		else
			this->BaseStream.reset();
	}

	void StreamWriter::Flush()
	{
		if (!this->BaseStream)
			IOError::StreamBaseStream();

		this->BaseStream->Flush();
	}

	void StreamWriter::Write(const char Value)
	{
		if (!this->BaseStream)
			IOError::StreamBaseStream();

		this->BaseStream->Write((uint8_t*)&Value, 0, 1);
	}

	void StreamWriter::Write(const char* Buffer, uint32_t Index, uint32_t Count)
	{
		if (!this->BaseStream)
			IOError::StreamBaseStream();

		this->BaseStream->Write((uint8_t*)&Buffer[0], Index, Count);
	}

	void StreamWriter::Write(const string& Value)
	{
		this->Write((const char*)Value, 0, Value.Length());
	}

	void StreamWriter::WriteLine(const string& Value)
	{
		TextWriter::WriteLine((const char*)Value, 0, Value.Length());
	}

	void StreamWriter::Write(const char* Value)
	{
		this->Write(Value, 0, (uint32_t)strlen(Value));
	}

	void StreamWriter::WriteLine(const char* Value)
	{
		if (Value == nullptr)
			TextWriter::WriteLine(nullptr, 0, 0);
		else
			TextWriter::WriteLine(Value, 0, (uint32_t)strlen(Value));
	}

	void StreamWriter::WriteFmt(const char* Format, ...)
	{
		va_list vArgs;
		va_start(vArgs, Format);

		char StackFmt[FormatBufferSize];
		auto ResultFmt = vsnprintf(StackFmt, FormatBufferSize, Format, vArgs);

		if (ResultFmt > 0 && ResultFmt < FormatBufferSize)
		{
			va_end(vArgs);
			this->Write((const char*)StackFmt, 0, (uint32_t)ResultFmt);
			return;
		}

		auto HeapFmt = std::make_unique<char[]>(ResultFmt + 1);

		vsnprintf(HeapFmt.get(), ResultFmt + 1, Format, vArgs);
		va_end(vArgs);

		this->Write((const char*)HeapFmt.get(), 0, (uint32_t)ResultFmt);
	}

	void StreamWriter::WriteLineFmt(const char* Format, ...)
	{
		if (Format == nullptr)
		{
			TextWriter::WriteLine(nullptr, 0, 0);
		}
		else
		{
			va_list vArgs;
			va_start(vArgs, Format);

			char StackFmt[FormatBufferSize];
			auto ResultFmt = vsnprintf(StackFmt, FormatBufferSize, Format, vArgs);

			if (ResultFmt > 0 && ResultFmt < FormatBufferSize)
			{
				va_end(vArgs);
				TextWriter::WriteLine((const char*)StackFmt, 0, (uint32_t)ResultFmt);
				return;
			}

			auto HeapFmt = std::make_unique<char[]>(ResultFmt + 1);

			vsnprintf(HeapFmt.get(), ResultFmt + 1, Format, vArgs);
			va_end(vArgs);

			TextWriter::WriteLine((const char*)HeapFmt.get(), 0, (uint32_t)ResultFmt);
		}
	}

	Stream* StreamWriter::GetBaseStream() const
	{
		return this->BaseStream.get();
	}
}