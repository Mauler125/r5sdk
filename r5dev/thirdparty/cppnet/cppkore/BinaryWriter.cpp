#include "stdafx.h"
#include "BinaryWriter.h"

namespace IO
{
	BinaryWriter::BinaryWriter()
		: BinaryWriter(nullptr, false)
	{
	}

	BinaryWriter::BinaryWriter(std::unique_ptr<Stream> Stream)
		: BinaryWriter(std::move(Stream), false)
	{
	}

	BinaryWriter::BinaryWriter(std::unique_ptr<Stream> Stream, bool LeaveOpen)
	{
		this->BaseStream = std::move(Stream);
		this->_LeaveOpen = LeaveOpen;
	}

	BinaryWriter::BinaryWriter(Stream* Stream)
		: BinaryWriter(Stream, false)
	{
	}

	BinaryWriter::BinaryWriter(Stream * Stream, bool LeaveOpen)
	{
		this->BaseStream.reset(Stream);
		this->_LeaveOpen = LeaveOpen;
	}

	BinaryWriter::~BinaryWriter()
	{
		this->Close();
	}

	void BinaryWriter::Write(std::unique_ptr<uint8_t[]>& Buffer, uint64_t Count)
	{
		this->Write(Buffer.get(), 0, Count);
	}

	void BinaryWriter::Write(uint8_t* Buffer, uint64_t Index, uint64_t Count)
	{
		if (!this->BaseStream)
			IOError::StreamBaseStream();

		this->BaseStream->Write(Buffer, Index, Count);
	}

	void BinaryWriter::Write(void* Buffer, uint64_t Index, uint64_t Count)
	{
		if (!this->BaseStream)
			IOError::StreamBaseStream();

		this->BaseStream->Write((uint8_t*)Buffer, Index, Count);
	}

	void BinaryWriter::WriteCString(const string& Value)
	{
		if (!this->BaseStream)
			IOError::StreamBaseStream();

		uint8_t NullBuffer = 0x0;
		auto ValueLength = Value.Length();

		this->BaseStream->Write((uint8_t*)Value.begin(), 0, ValueLength);

		if (ValueLength == 0 || Value[ValueLength - 1] != '\0')
			this->BaseStream->Write(&NullBuffer, 0, 1);
	}

	void BinaryWriter::WriteWCString(const wstring& Value)
	{
		if (!this->BaseStream)
			IOError::StreamBaseStream();

		uint16_t NullBuffer = 0x0;
		auto ValueLength = Value.Length();

		this->BaseStream->Write((uint8_t*)Value.begin(), 0, ValueLength * sizeof(wchar_t));

		if (ValueLength == 0 || Value[ValueLength - 1] != (wchar_t)'\0')
			this->BaseStream->Write((uint8_t*)&NullBuffer, 0, sizeof(uint16_t));
	}

	void BinaryWriter::WriteSizeString(const string& Value)
	{
		if (!this->BaseStream)
			IOError::StreamBaseStream();

		this->BaseStream->Write((uint8_t*)Value.begin(), 0, Value.Length());
	}

	void BinaryWriter::WriteNetString(const string& Value)
	{
		if (!this->BaseStream)
			IOError::StreamBaseStream();

		auto ValueSize = (uint32_t)Value.Length();
		this->WriteVarInt(ValueSize);

		this->BaseStream->Write((uint8_t*)Value.begin(), 0, ValueSize);
	}

	void BinaryWriter::WriteVarInt(uint32_t Value)
	{
		// Write out 7 bits per byte, highest bit = keep reading
		while (Value >= 0x80)
		{
			this->Write<uint8_t>((uint8_t)(Value | 0x80));
			Value >>= 7;
		}

		this->Write<uint8_t>((uint8_t)Value);
	}

	void BinaryWriter::Pad(uint64_t Count)
	{
		char Padding[0x1000]{};

		while (Count > 0)
		{
			auto Want = (Count > 0x1000) ? 0x1000 : Count;

			Write(Padding, 0, Want);

			Count -= Want;
		}
	}

	Stream* BinaryWriter::GetBaseStream() const
	{
		return this->BaseStream.get();
	}

	void BinaryWriter::Close()
	{
		// Forcefully reset the stream
		if (this->_LeaveOpen)
			this->BaseStream.release();
		else
			this->BaseStream.reset();
	}
}