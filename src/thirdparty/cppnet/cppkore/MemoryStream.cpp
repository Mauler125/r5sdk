#include "stdafx.h"
#include "MemoryStream.h"

namespace IO
{
	MemoryStream::MemoryStream()
		: MemoryStream(0)
	{
	}

	MemoryStream::MemoryStream(uint64_t Capacity)
	{
		this->_Buffer = new uint8_t[Capacity];
		this->_BufferSize = Capacity;
		this->_Length = 0;
		this->_CanWrite = true;
		this->_Expandable = true;
		this->_Origin = 0;
		this->_KeepOpen = false;
	}

	MemoryStream::MemoryStream(uint8_t* Buffer, uint64_t Index, uint64_t Count)
		: MemoryStream(Buffer, Index, Count, true, false)
	{
	}

	MemoryStream::MemoryStream(uint8_t* Buffer, uint64_t Index, uint64_t Count, bool Writable)
		: MemoryStream(Buffer, Index, Count, Writable, false)
	{
	}

	MemoryStream::MemoryStream(uint8_t* Buffer, uint64_t Index, uint64_t Count, bool Writable, bool LeaveOpen)
		: MemoryStream(Buffer, Index, Count, Writable, LeaveOpen, false)
	{
	}

	MemoryStream::MemoryStream(uint8_t* Buffer, uint64_t Index, uint64_t Count, bool Writable, bool LeaveOpen, bool Expandable)
	{
		if (!Buffer)
			throw std::exception("The buffer must not be null");

		this->_Buffer = Buffer;
		this->_Origin = Index;
		this->_Position = Index;
		this->_Length = (Index + Count);
		this->_BufferSize = (Index + Count);
		this->_CanWrite = Writable;
		this->_KeepOpen = LeaveOpen;
		this->_Expandable = Expandable;
	}

	MemoryStream::~MemoryStream()
	{
		//this->Close();
	}

	bool MemoryStream::CanRead()
	{
		return (this->_Buffer);
	}

	bool MemoryStream::CanWrite()
	{
		return (this->_Buffer) ? this->_CanWrite : false;
	}

	bool MemoryStream::CanSeek()
	{
		return (this->_Buffer);
	}

	bool MemoryStream::GetIsEndOfFile()
	{
		if (!this->_Expandable)
			return (this->_Position == this->_Length);
		return false;
	}

	uint64_t MemoryStream::GetLength()
	{
		if (!this->_Buffer)
			throw std::exception("Stream not open");

		return (this->_Length - this->_Origin);
	}

	uint64_t MemoryStream::GetPosition()
	{
		if (!this->_Buffer)
			throw std::exception("Stream not open");

		return this->_Position;
	}

	void MemoryStream::SetLength(uint64_t Length)
	{
		auto nLength = this->_Origin + Length;
		EnsureCapacity(nLength);

		this->_Length = nLength;
		if (this->_Position > nLength)
			this->_Position = nLength;
	}

	void MemoryStream::SetPosition(uint64_t Position)
	{
		this->Seek(Position, SeekOrigin::Begin);
	}

	void MemoryStream::Close()
	{
		if (!this->_KeepOpen && this->_Buffer)
			delete[] this->_Buffer;

		this->_Buffer = nullptr;
		this->_KeepOpen = false;
		this->_BufferSize = 0;
		this->_CanWrite = false;
		this->_Expandable = true;
		this->_Length = 0;
		this->_Position = 0;
		this->_Origin = 0;
	}

	void MemoryStream::Flush()
	{
		// This is a non-cached buffer, flush does nothing
	}

	void MemoryStream::Seek(uint64_t Offset, SeekOrigin Origin)
	{
		if (!this->_Buffer)
			throw std::exception("Stream not open");

		auto nOffset = this->_Position;
		switch (Origin)
		{
		case SeekOrigin::Begin:
			nOffset = this->_Origin + Offset;
			if ((this->_Position + Offset) < this->_Origin || nOffset < this->_Origin)
				throw std::exception("Attempt to seek outside the bounds of the stream");
			this->_Position = nOffset;
			break;
		case SeekOrigin::Current:
			nOffset = this->_Position + Offset;
			if ((this->_Position + Offset) < this->_Origin || nOffset < this->_Origin)
				throw std::exception("Attempt to seek outside the bounds of the stream");
			this->_Position = nOffset;
			break;
		case SeekOrigin::End:
			nOffset = this->_Length - Offset;
			if ((this->_Length - Offset) < this->_Origin || nOffset < _Origin)
				throw std::exception("Attempt to seek outside the bounds of the stream");
			this->_Position = nOffset;
			break;
		}
	}

	uint64_t MemoryStream::Read(uint8_t* Buffer, uint64_t Offset, uint64_t Count)
	{
		if (!this->_Buffer)
			throw std::exception("Stream not open");

		// Ensure we are within the bounds of the buffer
		int64_t nLength = (int64_t)this->_Length - (int64_t)this->_Position;
		if (nLength > (int64_t)Count) nLength = Count;
		if (nLength <= 0)
			return 0;

		std::memcpy(Buffer + Offset, this->_Buffer + this->_Position, (size_t)nLength);
		this->_Position += nLength;

		return nLength;
	}

	uint64_t MemoryStream::Read(uint8_t* Buffer, uint64_t Offset, uint64_t Count, uint64_t Position)
	{
		this->SetPosition(Position);
		return this->Read(Buffer, Offset, Count);
	}

	void MemoryStream::Write(uint8_t* Buffer, uint64_t Offset, uint64_t Count)
	{
		if (!this->_Buffer)
			throw std::exception("Stream not open");
		if (!this->_CanWrite)
			throw std::exception("Write not supported");

		auto nBuffer = this->_Position + Count;
		if (nBuffer > this->_Length)
		{
			this->EnsureCapacity(nBuffer);
			this->_Length = nBuffer;
		}
		
		std::memcpy(this->_Buffer + this->_Position, Buffer + Offset, Count);
		this->_Position = nBuffer;
	}

	void MemoryStream::Write(uint8_t* Buffer, uint64_t Offset, uint64_t Count, uint64_t Position)
	{
		this->SetPosition(Position);
		this->Write(Buffer, Offset, Count);
	}

	void MemoryStream::EnsureCapacity(uint64_t Size)
	{
		if (Size < this->_BufferSize)
			return;
		if (!this->_Expandable)
			throw std::exception("Expand not supported");

		auto nCapacity = Size;
		if (nCapacity < 256)
			nCapacity = 256;

		if (nCapacity < this->_BufferSize * 2)
			nCapacity = this->_BufferSize * 2;

		auto tBuffer = this->_Buffer;
		this->_Buffer = new uint8_t[nCapacity];

		std::memcpy(this->_Buffer, tBuffer, this->_BufferSize);
		this->_BufferSize = nCapacity;

		delete[] tBuffer;
	}
}