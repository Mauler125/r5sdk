#include "stdafx.h"
#include "DeflateStream.h"

#include "..\cppkore_incl\ZLib\miniz.h"

#if _DEBUG
#pragma comment(lib, "..\\cppkore_libs\\ZLib\\cppkorezlibx64d.lib")
#else
#pragma comment(lib, "..\\cppkore_libs\\ZLib\\cppkorezlibx64r.lib")
#endif

namespace Compression
{
	DeflateStream::DeflateStream(std::unique_ptr<IO::Stream> Stream, CompressionMode Mode, bool LeaveOpen)
		: BaseStream(std::move(Stream)), _Mode(Mode), _LeaveOpen(LeaveOpen), _BufferLength(DeflateStream::DefaultBufferSize), _BufferOffset(0)
	{
		this->_Buffer = std::make_unique<uint8_t[]>(DeflateStream::DefaultBufferSize);
		this->CreateInflatorDeflator();
	}

	DeflateStream::DeflateStream(IO::Stream* Stream, CompressionMode Mode, bool LeaveOpen)
		: BaseStream(Stream), _Mode(Mode), _LeaveOpen(LeaveOpen), _BufferLength(DeflateStream::DefaultBufferSize), _BufferOffset(0)
	{
		this->_Buffer = std::make_unique<uint8_t[]>(DeflateStream::DefaultBufferSize);
		this->CreateInflatorDeflator();
	}

	DeflateStream::~DeflateStream()
	{
		this->Close();
	}

	bool DeflateStream::CanRead()
	{
		return (this->_Mode == CompressionMode::Decompress);
	}

	bool DeflateStream::CanWrite()
	{
		return (this->_Mode == CompressionMode::Compress);
	}

	bool DeflateStream::CanSeek()
	{
		return (_Mode == CompressionMode::Decompress);
	}

	bool DeflateStream::GetIsEndOfFile()
	{
		return this->BaseStream->GetIsEndOfFile();
	}

	uint64_t DeflateStream::GetLength()
	{
		return this->BaseStream->GetLength();
	}

	uint64_t DeflateStream::GetPosition()
	{
		return this->BaseStream->GetPosition();
	}

	void DeflateStream::SetLength(uint64_t Length)
	{
		IO::IOError::StreamSetLengthSupport();
	}

	void DeflateStream::SetPosition(uint64_t Position)
	{
		if (_Mode == CompressionMode::Decompress)
		{
			this->BaseStream->SetPosition(Position);
			mz_inflateReset((mz_streamp)this->_DeflateState);
		}
		else
			IO::IOError::StreamNoSeekSupport();
	}

	void DeflateStream::Close()
	{
		if (this->_Mode == CompressionMode::Compress)
			this->WriteDeflaterOutput();	// Ensure that all data has been written to the stream...

		if (this->_LeaveOpen)
			this->BaseStream.release();
		else
			this->BaseStream.reset();

		this->_Buffer.reset();

		if (this->_Mode == CompressionMode::Compress)
			deflateEnd((mz_streamp)this->_DeflateState);
		else
			inflateEnd((mz_streamp)this->_DeflateState);

		delete (mz_streamp)this->_DeflateState;
	}

	void DeflateStream::Flush()
	{
		if (this->CanWrite())
			this->BaseStream->Flush();
	}

	void DeflateStream::Seek(uint64_t Offset, IO::SeekOrigin Origin)
	{
		if (_Mode == CompressionMode::Decompress)
		{
			this->BaseStream->Seek(Offset, Origin);
			mz_inflateReset((mz_streamp)this->_DeflateState);
		}
		else
			IO::IOError::StreamNoSeekSupport();
	}

	uint64_t DeflateStream::Read(uint8_t* Buffer, uint64_t Offset, uint64_t Count)
	{
		uint64_t TotalRead = 0;
		uint64_t TotalOutNow = 0;

		mz_streamp StreamState = (mz_streamp)this->_DeflateState;

		TotalOutNow = StreamState->total_out;

		while (true)
		{
			StreamState->avail_out = (unsigned int)(Count - TotalRead);
			StreamState->next_out = (unsigned char*)(Buffer + Offset + TotalRead);
			
			auto Result = inflate(StreamState, MZ_SYNC_FLUSH);
			TotalRead = StreamState->total_out - TotalOutNow;

			if (TotalRead == Count)
				break;

			if (Result == MZ_STREAM_END)
				break;

			auto RequiredRead = (StreamState->avail_in > 0) ? (DeflateStream::DefaultBufferSize - StreamState->avail_in) : DeflateStream::DefaultBufferSize;

			//
			// We must move the z_stream expected block to the front, and then read expected input after...
			//

			std::memmove(this->_Buffer.get(), this->_Buffer.get() + RequiredRead, StreamState->avail_in);

			auto Bytes = this->BaseStream->Read(this->_Buffer.get(), StreamState->avail_in, RequiredRead);

			if (Bytes == 0)
				break;

			StreamState->next_in = (const unsigned char*)this->_Buffer.get();
			StreamState->avail_in = (unsigned int)DeflateStream::DefaultBufferSize;
		}

		return TotalRead;
	}

	uint64_t DeflateStream::Read(uint8_t* Buffer, uint64_t Offset, uint64_t Count, uint64_t Position)
	{
		return 0;
	}

	void DeflateStream::Write(uint8_t* Buffer, uint64_t Offset, uint64_t Count)
	{
		// Continue to write if need be
		this->WriteDeflaterOutput();

		// Set the new input and size to us
		mz_streamp StreamState = (mz_streamp)this->_DeflateState;

		StreamState->avail_in = (unsigned int)Count;
		StreamState->next_in = (const unsigned char*)(Buffer + Offset);

		// Write the last output
		this->WriteDeflaterOutput();
	}

	void DeflateStream::Write(uint8_t* Buffer, uint64_t Offset, uint64_t Count, uint64_t Position)
	{
	}

	void DeflateStream::CreateInflatorDeflator()
	{
		this->_DeflateState = new z_stream();

		if (_Mode == CompressionMode::Compress)
			deflateInit2((mz_streamp)this->_DeflateState, MZ_DEFAULT_LEVEL, MZ_DEFLATED, -MZ_DEFAULT_WINDOW_BITS, 9, MZ_DEFAULT_STRATEGY);
		else if (_Mode == CompressionMode::Decompress)
			inflateInit2((mz_streamp)this->_DeflateState, -MZ_DEFAULT_WINDOW_BITS);

		mz_streamp StreamState = (mz_streamp)this->_DeflateState;

		// Setup default state values...
		StreamState->avail_in = 0;
		StreamState->avail_out = 0;
		StreamState->next_in = nullptr;
		StreamState->next_out = nullptr;
	}

	void DeflateStream::WriteDeflaterOutput()
	{
		mz_streamp StreamState = (mz_streamp)this->_DeflateState;

		// Loop until we need data again...
		while (StreamState->avail_in > 0)
		{
			// Reset the buffers
			StreamState->next_out = (unsigned char*)this->_Buffer.get();
			StreamState->avail_out = (unsigned int)DeflateStream::DefaultBufferSize;

			// Prepare to deflate the data
			auto Result = deflate(StreamState, MZ_SYNC_FLUSH);

			// Write the data if any
			this->BaseStream->Write(this->_Buffer.get(), 0, (DeflateStream::DefaultBufferSize - StreamState->avail_out));
		}
	}
}
