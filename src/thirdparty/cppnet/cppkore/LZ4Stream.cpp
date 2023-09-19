#include "stdafx.h"
#include "LZ4Stream.h"

#include "..\cppkore_incl\LZ4_XXHash\lz4.h"
#include "..\cppkore_incl\LZ4_XXHash\lz4hc.h"

#if _WIN64
#pragma comment(lib, "..\\cppkore_libs\\LZ4_XXHash\\liblz4_static_64.lib")
#else
#pragma comment(lib, "..\\cppkore_libs\\LZ4_XXHash\\liblz4_static_32.lib")
#endif

namespace Compression
{
	LZ4Stream::LZ4Stream(std::unique_ptr<IO::Stream> Stream, CompressionMode Mode, bool HighCompression, uint32_t BlockSize, bool LeaveOpen)
		: BaseStream(std::move(Stream)), _Mode(Mode), _HighCompression(HighCompression), _BlockSize(BlockSize), _LeaveOpen(LeaveOpen)
	{
	}

	LZ4Stream::LZ4Stream(IO::Stream* Stream, CompressionMode Mode, bool HighCompression, uint32_t BlockSize, bool LeaveOpen)
		: BaseStream(Stream), _Mode(Mode), _HighCompression(HighCompression), _BlockSize(BlockSize), _LeaveOpen(LeaveOpen)
	{
	}

	LZ4Stream::~LZ4Stream()
	{
		this->Close();
	}

	bool LZ4Stream::CanRead()
	{
		return (this->_Mode == CompressionMode::Decompress);
	}

	bool LZ4Stream::CanWrite()
	{
		return (this->_Mode == CompressionMode::Compress);
	}

	bool LZ4Stream::CanSeek()
	{
		return false;
	}

	bool LZ4Stream::GetIsEndOfFile()
	{
		return this->BaseStream->GetIsEndOfFile();
	}

	uint64_t LZ4Stream::GetLength()
	{
		return this->BaseStream->GetLength();
	}

	uint64_t LZ4Stream::GetPosition()
	{
		return this->BaseStream->GetPosition();
	}

	void LZ4Stream::SetLength(uint64_t Length)
	{
		IO::IOError::StreamSetLengthSupport();
	}

	void LZ4Stream::SetPosition(uint64_t Position)
	{
		IO::IOError::StreamNoSeekSupport();
	}

	void LZ4Stream::Close()
	{
		if (this->_LeaveOpen)
			this->BaseStream.release();
		else
			this->BaseStream.reset();

		this->_Buffer.reset();
	}

	void LZ4Stream::Flush()
	{
		if (this->CanWrite())
			this->BaseStream->Flush();
	}

	void LZ4Stream::Seek(uint64_t Offset, IO::SeekOrigin Origin)
	{
		IO::IOError::StreamNoSeekSupport();
	}

	uint64_t LZ4Stream::Read(uint8_t* Buffer, uint64_t Offset, uint64_t Count)
	{
		uint64_t TotalRead = 0;

		while (Count > 0)
		{
			auto Chunk = std::min<uint64_t>(Count, this->_BufferLength - this->_BufferOffset);

			if (Chunk > 0)
			{
				std::memcpy(Buffer + Offset, this->_Buffer.get() + this->_BufferOffset, Chunk);

				this->_BufferOffset += (uint32_t)Chunk;
				TotalRead += Chunk;

				Offset += Chunk;
				Count -= Chunk;
			}
			else
			{
				if (!this->AcquireNextChunk())
					break;
			}
		}

		return TotalRead;
	}

	uint64_t LZ4Stream::Read(uint8_t* Buffer, uint64_t Offset, uint64_t Count, uint64_t Position)
	{
		return 0;
	}

	void LZ4Stream::Write(uint8_t* Buffer, uint64_t Offset, uint64_t Count)
	{
		if (this->_Buffer == nullptr)
		{
			this->_Buffer.reset(new uint8_t[this->_BlockSize]);
			this->_BufferOffset = 0;
			this->_BufferLength = this->_BlockSize;
		}
		
		while (Count > 0)
		{
			auto Chunk = std::min<uint64_t>(Count, this->_BufferLength - this->_BufferOffset);

			if (Chunk > 0)
			{
				std::memcpy(this->_Buffer.get() + this->_BufferOffset, Buffer + Offset, Chunk);

				Offset += Chunk;
				Count -= Chunk;
				this->_BufferOffset += (uint32_t)Chunk;
			}
			else
			{
				this->FlushCurrentChunk();
			}
		}
	}

	void LZ4Stream::Write(uint8_t* Buffer, uint64_t Offset, uint64_t Count, uint64_t Position)
	{
	}

	bool LZ4Stream::AcquireNextChunk()
	{
		do
		{
			uint64_t VarInt;
			if (!this->TryReadVarInt(VarInt))
				return false;

			bool isCompressed = (VarInt & 0x1) != 0;

			auto DecompressedLength = this->ReadVarInt();
			auto CompressedLength = (isCompressed) ? this->ReadVarInt() : DecompressedLength;

			if (!isCompressed)
			{
				if (this->_BufferLength < CompressedLength || this->_Buffer == nullptr)
					this->_Buffer.reset(new uint8_t[CompressedLength]);

				this->BaseStream->Read(this->_Buffer.get(), 0, CompressedLength);
				this->_BufferLength = (uint32_t)CompressedLength;
			}
			else
			{
				if (this->_BufferLength < DecompressedLength || this->_Buffer == nullptr)
					this->_Buffer.reset(new uint8_t[DecompressedLength]);

				auto TempBuffer = std::make_unique<uint8_t[]>(CompressedLength);
				this->BaseStream->Read(TempBuffer.get(), 0, CompressedLength);

				LZ4_decompress_safe((const char*)TempBuffer.get(), (char*)this->_Buffer.get(), (int)CompressedLength, (int)DecompressedLength);

				this->_BufferLength = (uint32_t)DecompressedLength;
			}

			this->_BufferOffset = 0;

		} while (this->_BufferLength == 0);

		return true;
	}

	bool LZ4Stream::TryReadVarInt(uint64_t& Result)
	{
		uint8_t Buffer;
		uint32_t Count = 0;

		Result = 0;

		while (true)
		{
			if (this->BaseStream->Read(&Buffer, 0, 1) == 0)
				return false;

			Result = Result + ((uint64_t)(Buffer & 0x7F) << Count);
			Count += 7;

			if ((Buffer & 0x80) == 0 || Count >= 64)
				break;
		}

		return true;
	}

	uint64_t LZ4Stream::ReadVarInt()
	{
		uint64_t Result = 0;

		if (!TryReadVarInt(Result))
			return 0;

		return Result;
	}

	void LZ4Stream::FlushCurrentChunk()
	{
		if (this->_BufferOffset == 0)
			return;

		auto TempBuffer = std::make_unique<uint8_t[]>(this->_BufferOffset);
		auto TempSize = (this->_HighCompression) ?
			LZ4_compress_HC((const char*)this->_Buffer.get(), (char*)TempBuffer.get(), this->_BufferOffset, this->_BufferOffset, LZ4HC_CLEVEL_MAX)
			: LZ4_compress_default((const char*)this->_Buffer.get(), (char*)TempBuffer.get(), this->_BufferOffset, this->_BufferOffset);

		if (TempSize <= 0 || (uint32_t)TempSize >= this->_BufferOffset)
		{
			// No compression...
			this->WriteVarInt(0);
			this->WriteVarInt(this->_BufferOffset);

			this->BaseStream->Write(this->_Buffer.get(), 0, this->_BufferOffset);
		}
		else
		{
			// Use compression...
			this->WriteVarInt(1);
			this->WriteVarInt(this->_BufferOffset);
			this->WriteVarInt(TempSize);

			this->BaseStream->Write(TempBuffer.get(), 0, TempSize);
		}

		this->_BufferOffset = 0;
	}

	void LZ4Stream::WriteVarInt(uint64_t Value)
	{
		uint8_t Buffer = 0;

		while (true)
		{
			auto Temp = (uint8_t)(Value & 0x7F);
			Value >>= 7;

			Buffer = (uint8_t)(Temp | (Value == 0 ? 0 : 0x80));

			this->BaseStream->Write(&Buffer, 0, 1);

			if (Value == 0)
				break;
		}
	}
}
