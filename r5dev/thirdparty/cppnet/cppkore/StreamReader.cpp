#include "stdafx.h"
#include "StreamReader.h"

namespace IO
{
	StreamReader::StreamReader()
		: StreamReader(nullptr, false)
	{
	}

	StreamReader::StreamReader(std::unique_ptr<Stream> Stream)
		: StreamReader(std::move(Stream), false)
	{
	}

	StreamReader::StreamReader(std::unique_ptr<Stream> Stream, bool LeaveOpen)
		: _CharLen(0), _CharPos(0)
	{
		this->BaseStream = std::move(Stream);
		this->_LeaveOpen = LeaveOpen;
		this->_InternalBuffer = std::make_unique<char[]>(StreamReader::DefaultCharBufferSize);
	}

	StreamReader::StreamReader(Stream* Stream)
		: StreamReader(Stream, false)
	{
	}

	StreamReader::StreamReader(Stream* Stream, bool LeaveOpen)
		: _CharLen(0), _CharPos(0)
	{
		this->BaseStream.reset(Stream);
		this->_LeaveOpen = LeaveOpen;
		this->_InternalBuffer = std::make_unique<char[]>(StreamReader::DefaultCharBufferSize);
	}

	StreamReader::~StreamReader()
	{
		this->Close();
	}

	void StreamReader::Close()
	{
		// Forcefully reset the stream
		if (this->_LeaveOpen)
			this->BaseStream.release();
		else
			this->BaseStream.reset();

		// Always clean up the internal buffer
		this->_InternalBuffer.reset();
	}

	int32_t StreamReader::Peek()
	{
		if (!this->BaseStream)
			IOError::StreamBaseStream();

		if (this->_CharPos == this->_CharLen)
		{
			if (ReadBuffer() == 0)
				return -1;
		}

		return this->_InternalBuffer[this->_CharPos];
	}

	int32_t StreamReader::Read()
	{
		if (!this->BaseStream)
			IOError::StreamBaseStream();

		if (this->_CharPos == this->_CharLen)
		{
			if (ReadBuffer() == 0)
				return -1;
		}

		return this->_InternalBuffer[this->_CharPos++];
	}

	Stream* StreamReader::GetBaseStream() const
	{
		return this->BaseStream.get();
	}

	int32_t StreamReader::ReadBuffer()
	{
		this->_CharLen = 0;
		this->_CharPos = 0;

		auto ReadResult = this->BaseStream->Read((uint8_t*)this->_InternalBuffer.get(), 0, StreamReader::DefaultCharBufferSize);

		// TODO: Eventually, we need to add support for BOM detection on first read, then, decoding from non UTF-8 BOM to UTF-8 for StringBase<char>
		// This will be the same as preamble checks in StreamReader.cs
		
		this->_CharLen = (uint32_t)ReadResult;
		return this->_CharLen;
	}
}