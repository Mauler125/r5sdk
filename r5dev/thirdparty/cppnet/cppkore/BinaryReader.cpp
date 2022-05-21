#include "stdafx.h"
#include "BinaryReader.h"
#include "Pattern.h"

namespace IO
{
	BinaryReader::BinaryReader()
		: BinaryReader(nullptr, false)
	{
	}

	BinaryReader::BinaryReader(std::unique_ptr<Stream> Stream)
		: BinaryReader(std::move(Stream), false)
	{
	}

	BinaryReader::BinaryReader(std::unique_ptr<Stream> Stream, bool LeaveOpen)
	{
		this->BaseStream = std::move(Stream);
		this->_LeaveOpen = LeaveOpen;
	}

	BinaryReader::BinaryReader(Stream* Stream)
		: BinaryReader(Stream, false)
	{
	}

	BinaryReader::BinaryReader(Stream* Stream, bool LeaveOpen)
	{
		this->BaseStream.reset(Stream);
		this->_LeaveOpen = LeaveOpen;
	}

	BinaryReader::~BinaryReader()
	{
		this->Close();
	}

	std::unique_ptr<uint8_t[]> BinaryReader::Read(uint64_t Count, uint64_t& Result)
	{
		auto Buffer = std::make_unique<uint8_t[]>(Count);

		Result = Read(Buffer.get(), 0, Count);

		return Buffer;
	}

	uint64_t BinaryReader::Read(uint8_t* Buffer, uint64_t Index, uint64_t Count)
	{
		if (!this->BaseStream)
			IOError::StreamBaseStream();

		return this->BaseStream->Read(Buffer, Index, Count);
	}

	uint64_t BinaryReader::Read(void* Buffer, uint64_t Index, uint64_t Count)
	{
		if (!this->BaseStream)
			IOError::StreamBaseStream();

		return this->BaseStream->Read((uint8_t*)Buffer, Index, Count);
	}

	string BinaryReader::ReadCString()
	{
		if (!this->BaseStream)
			IOError::StreamBaseStream();

		string Buffer = "";
		
		char Cur = this->Read<char>();
		while ((uint8_t)Cur > 0)
		{
			Buffer += Cur;
			Cur = this->Read<char>();
		}

		return std::move(Buffer);
	}

	wstring BinaryReader::ReadWCString()
	{
		if (!this->BaseStream)
			IOError::StreamBaseStream();

		wstring Buffer = L"";

		wchar_t Cur = this->Read<wchar_t>();
		while (Cur != (wchar_t)'\0')
		{
			Buffer += Cur;
			Cur = this->Read<wchar_t>();
		}

		return std::move(Buffer);
	}

	string BinaryReader::ReadSizeString(uint64_t Size)
	{
		if (!this->BaseStream)
			IOError::StreamBaseStream();

		auto Buffer = string((uint32_t)Size, '\0');
		this->BaseStream->Read((uint8_t*)&Buffer[0], 0, Size);

		return std::move(Buffer);
	}

	string BinaryReader::ReadNetString()
	{
		if (!this->BaseStream)
			IOError::StreamBaseStream();

		auto Buffer = string(this->ReadVarInt(), '\0');
		this->BaseStream->Read((uint8_t*)&Buffer[0], 0, (uint64_t)Buffer.Length());

		return std::move(Buffer);
	}

	uint32_t BinaryReader::ReadVarInt()
	{
		uint32_t Count = 0, Shift = 0;
		uint8_t Byte;

		do
		{
			if (Shift == 5 * 7)
				return 0;

			Byte = this->Read<uint8_t>();
			Count |= (Byte & 0x7F) << Shift;
			Shift += 7;
		} while ((Byte & 0x80) != 0);

		return Count;
	}

	int64_t BinaryReader::SignatureScan(const string& Signature)
	{
		if (!this->BaseStream)
			IOError::StreamBaseStream();

		auto Sig = Data::Pattern(Signature);
		auto Position = this->BaseStream->GetPosition();
		auto Length = this->BaseStream->GetLength();
		auto ChunkSize = (0x5F5E100 + (0x5F5E100 % Sig.DataSize()));

		uint64_t SearchResult = -1, ReadResult = 0, DataRead = 0;

		while (true)
		{
			auto StartPosition = this->BaseStream->GetPosition();
			auto Buffer = this->Read(ChunkSize, ReadResult);

			auto ChunkResult = Sig.Search(Buffer.get(), 0, ReadResult);
			if (ChunkResult > -1)
			{
				return (DataRead + ChunkResult + StartPosition);
			}
			DataRead += ReadResult;

			if (ReadResult < ChunkSize)
				break;
		}

		return SearchResult;
	}

	int64_t BinaryReader::SignatureScan(const string& Signature, uint64_t Offset, uint64_t Count)
	{
		if (!this->BaseStream)
			IOError::StreamBaseStream();

		uint64_t ReadResult = 0;
		auto Sig = Data::Pattern(Signature);
		this->BaseStream->SetPosition(Offset);
		auto Buffer = this->Read(Count, ReadResult);

		auto SearchResult = Sig.Search(Buffer.get(), 0, ReadResult);

		if (SearchResult > -1)
			return (SearchResult + Offset);

		return SearchResult;
	}

	List<int64_t> BinaryReader::SignatureScanAll(const string & Signature)
	{
		if (!this->BaseStream)
			IOError::StreamBaseStream();

		auto ResultList = List<int64_t>();

		auto Sig = Data::Pattern(Signature);
		auto Position = this->BaseStream->GetPosition();
		auto Length = this->BaseStream->GetLength();
		auto ChunkSize = (0x5F5E100 + (0x5F5E100 % Sig.DataSize()));

		uint64_t SearchResult = -1, ReadResult = 0, DataRead = 0;

		while (true)
		{
			auto StartPosition = this->BaseStream->GetPosition();
			auto Buffer = this->Read(ChunkSize, ReadResult);

			auto ChunkResult = Sig.SearchAll(Buffer.get(), 0, ReadResult);
			
			for (auto& Result : ChunkResult)
				ResultList.EmplaceBack(Result + DataRead + StartPosition);

			DataRead += ReadResult;

			if (ReadResult < ChunkSize)
				break;
		}

		return ResultList;
	}

	List<int64_t> BinaryReader::SignatureScanAll(const string & Signature, uint64_t Offset, uint64_t Count)
	{
		if (!this->BaseStream)
			IOError::StreamBaseStream();

		uint64_t ReadResult = 0;
		auto Sig = Data::Pattern(Signature);
		this->BaseStream->SetPosition(Offset);
		auto Buffer = this->Read(Count, ReadResult);
		
		auto ResultList = Sig.SearchAll(Buffer.get(), 0, ReadResult);

		for (auto& Result : ResultList)
			Result += Offset;

		return ResultList;
	}

	Stream* BinaryReader::GetBaseStream() const
	{
		return this->BaseStream.get();
	}

	void BinaryReader::Close()
	{
		// Forcefully reset the stream
		if (this->_LeaveOpen)
			this->BaseStream.release();
		else
			this->BaseStream.reset();
	}
}
