#include "stdafx.h"
#include "Path.h"
#include "File.h"
#include "Directory.h"
#include "BinaryReader.h"
#include "BinaryWriter.h"
#include "ZipArchive.h"
#include "ZLibCodec.h"
#include "DeflateStream.h"

namespace Compression
{
	ZipArchive::ZipArchive()
		: ZipArchive(nullptr, false)
	{
	}

	ZipArchive::ZipArchive(std::unique_ptr<IO::Stream> Stream)
		: ZipArchive(std::move(Stream), false)
	{
	}

	ZipArchive::ZipArchive(std::unique_ptr<IO::Stream> Stream, bool LeaveOpen)
	{
		this->BaseStream = std::move(Stream);
		this->_LeaveOpen = LeaveOpen;
		this->_ExistingFiles = 0;
		this->_CentralDirLength = 0;

		this->ReadFileInfo();
	}

	ZipArchive::ZipArchive(IO::Stream* Stream)
		: ZipArchive(Stream, false)
	{
	}

	ZipArchive::ZipArchive(IO::Stream* Stream, bool LeaveOpen)
	{
		this->BaseStream.reset(Stream);
		this->_LeaveOpen = LeaveOpen;
		this->_ExistingFiles = 0;
		this->_CentralDirLength = 0;

		this->ReadFileInfo();
	}

	ZipArchive::~ZipArchive()
	{
		this->Close();
	}

	List<ZipEntry> ZipArchive::ReadEntries()
	{
		auto Result = List<ZipEntry>();

		if (this->_CentralDir == nullptr)
			return Result;

		uint8_t* Buffer = this->_CentralDir.get();

		for (uint64_t i = 0; i < this->_CentralDirLength;)
		{
			auto Sig = *(uint32_t*)(&Buffer[i]);

			if (Sig != 0x02014b50)
				break;

			bool EncodeUTF8 = (*(uint16_t*)(&Buffer[i + 8]) & 0x800) != 0;
			uint16_t Method = *(uint16_t*)(&Buffer[i + 10]);
			uint32_t ModifyTime = *(uint32_t*)(&Buffer[i + 12]);
			uint32_t Crc32 = *(uint32_t*)(&Buffer[i + 16]);
			uint64_t CompressedSize = *(uint32_t*)(&Buffer[i + 20]);
			uint64_t FileSize = *(uint32_t*)(&Buffer[i + 24]);
			uint16_t FileNameSize = *(uint16_t*)(&Buffer[i + 28]);
			uint16_t ExtraSize = *(uint16_t*)(&Buffer[i + 30]);
			uint16_t CommentSize = *(uint16_t*)(&Buffer[i + 32]);
			uint32_t HeaderOffset = *(uint32_t*)(&Buffer[i + 42]);
			uint32_t HeaderSize = (uint32_t)(46 + FileNameSize + ExtraSize + CommentSize);

			auto Entry = ZipEntry();

			Entry.Method = (ZipCompressionMethod)Method;
			Entry.FileNameInZip = string((const char*)&Buffer[i + 46], (size_t)FileNameSize);
			Entry.FileOffset = GetFileOffset(HeaderOffset);
			Entry.FileSize = FileSize;
			Entry.CompressedSize = CompressedSize;
			Entry.HeaderOffset = HeaderOffset;
			Entry.HeaderSize = HeaderSize;
			Entry.Crc32 = Crc32;
			Entry.EncodeUTF8 = EncodeUTF8;

			if (CommentSize > 0)
				Entry.Comment = string((const char*)&Buffer[i + 46 + FileNameSize + ExtraSize], (size_t)CommentSize);

			if (ExtraSize > 0)
				this->ReadExtraInfo(i + 46 + FileNameSize, Entry);

			Result.EmplaceBack(std::move(Entry));

			i += (46l + (uint64_t)FileNameSize + (uint64_t)ExtraSize + (uint64_t)CommentSize);
		}

		return Result;
	}

	ZipEntry ZipArchive::AddFile(ZipCompressionMethod Method, const string& Path, const string& FileNameInZip, const string& Comment)
	{
		auto Fs = IO::File::OpenRead(Path);
		return std::move(AddStream(Method, FileNameInZip, Fs.get(), Comment));
	}

	ZipEntry ZipArchive::AddStream(ZipCompressionMethod Method, const string& FileNameInZip, IO::Stream* Stream, const string& Comment)
	{
		auto Entry = ZipEntry();

		Entry.Method = Method;
		Entry.EncodeUTF8 = false;
		Entry.FileNameInZip = NormalizeFileName(FileNameInZip);
		Entry.Comment = Comment;
		Entry.Crc32 = 0;
		Entry.HeaderOffset = this->BaseStream->GetPosition();

		// Write local header
		this->WriteLocalHeader(Entry);
		Entry.FileOffset = this->BaseStream->GetPosition();

		// TODO: Write file to zip (store)


		this->UpdateCrcAndSizes(Entry);
		// TODO: Add to files array...

		return std::move(Entry);
	}

	void ZipArchive::ExtractFile(ZipEntry& Entry, const string& FileName)
	{
		auto Path = IO::Path::GetDirectoryName(FileName);

		IO::Directory::CreateDirectory(Path);

		if (IO::Directory::Exists(FileName))
			return;

		auto FileStream = IO::File::Create(FileName);

		this->ExtractFile(Entry, FileStream.get());
	}

	void ZipArchive::ExtractFile(ZipEntry& Entry, IO::Stream* Stream)
	{
		this->BaseStream->SetPosition(Entry.HeaderOffset);

		uint32_t Magic = 0;
		this->BaseStream->Read((uint8_t*)&Magic, 0, 4);

		if (Magic != 0x04034b50)
			return;

		this->BaseStream->SetPosition(Entry.FileOffset);

		if (Entry.Method == ZipCompressionMethod::Store)
		{
			auto Buffer = std::make_unique<uint8_t[]>(65535);
			auto BytesPending = (uint64_t)Entry.FileSize;

			while (BytesPending > 0)
			{
				auto Result = this->BaseStream->Read(Buffer.get(), 0, min(BytesPending, 65535));
				Stream->Write(Buffer.get(), 0, Result);

				BytesPending -= Result;
			}
		}
		else if (Entry.Method == ZipCompressionMethod::Deflate)
		{
			auto InStream = DeflateStream(this->BaseStream.get(), CompressionMode::Decompress, true);

			auto Buffer = std::make_unique<uint8_t[]>(65535);
			auto BytesPending = (uint64_t)Entry.FileSize;

			while (BytesPending > 0)
			{
				auto Result = InStream.Read(Buffer.get(), 0, min(BytesPending, 65535));
				Stream->Write(Buffer.get(), 0, Result);

				BytesPending -= Result;
			}
		}
	}

	void ZipArchive::ExtractFile(ZipEntry& Entry, std::unique_ptr<IO::Stream>& Stream)
	{
		this->ExtractFile(Entry, Stream.get());
	}

	IO::Stream* ZipArchive::GetBaseStream() const
	{
		return this->BaseStream.get();
	}

	void ZipArchive::Close()
	{
		// TODO: Finalize writing
		// https://github.com/jaime-olivares/zipstorer/blob/master/src/ZipStorer.cs#L330

		// Forcefully reset the stream
		if (this->_LeaveOpen)
			this->BaseStream.release();
		else
			this->BaseStream.reset();

		// Just close the central directory
		this->_CentralDir.reset();
	}

	void ZipArchive::ReadFileInfo()
	{
		// This is the minimum size of a zip archive
		if (this->BaseStream->GetLength() < 22)
			return;

		try
		{
			this->BaseStream->SetPosition(this->BaseStream->GetLength() - 17);

			auto Reader = IO::BinaryReader(this->BaseStream.get(), true);

			do
			{
				this->BaseStream->SetPosition(this->BaseStream->GetPosition() - 5);

				auto Sig = Reader.Read<uint32_t>();
				
				if (Sig == 0x06054b50)
				{
					auto DirPosition = this->BaseStream->GetPosition() - 4;

					this->BaseStream->Seek(6, IO::SeekOrigin::Current);

					uint64_t Entries = Reader.Read<uint16_t>();
					uint64_t CentralSize = Reader.Read<uint32_t>();
					uint64_t CentralDirOffset = Reader.Read<uint32_t>();
					uint16_t CommentSize = Reader.Read<uint16_t>();

					auto CommentPosition = this->BaseStream->GetPosition();

					if (CentralDirOffset == 0xffffffff)	// We have a Zip64 file
					{
						this->BaseStream->SetPosition(DirPosition - 20);

						Sig = Reader.Read<uint32_t>();

						if (Sig != 0x07064b50)	// Invalid Zip64 central dir locator
							return;

						this->BaseStream->Seek(4, IO::SeekOrigin::Current);
						this->BaseStream->SetPosition(Reader.Read<uint64_t>());

						Sig = Reader.Read<uint32_t>();

						if (Sig != 0x06064b50)	// Not a Zip64 central dir record
							return;

						this->BaseStream->Seek(28, IO::SeekOrigin::Current);

						Entries = Reader.Read<uint64_t>();
						CentralSize = Reader.Read<uint64_t>();
						CentralDirOffset = Reader.Read<uint64_t>();
					}

					if (CommentPosition + CommentSize != this->BaseStream->GetLength())
						return;

					uint64_t Temp = 0;

					this->_ExistingFiles = Entries;

					this->BaseStream->SetPosition(CentralDirOffset);

					this->_CentralDirLength = CentralSize;
					this->_CentralDir = Reader.Read(CentralSize, Temp);

					this->BaseStream->SetPosition(CentralDirOffset);
					return;
				}

			} while (this->BaseStream->GetPosition() > 0);
		}
		catch (...)
		{
			// An error occured while reading the central directory
		}
	}

	uint32_t ZipArchive::GetFileOffset(uint32_t HeaderOffset)
	{
		uint8_t Buffer[2]{};

		this->BaseStream->SetPosition(HeaderOffset + 26);
		this->BaseStream->Read(Buffer, 0, 2);

		uint16_t FileNameSize = *(uint16_t*)&Buffer[0];

		this->BaseStream->Read(Buffer, 0, 2);

		uint16_t ExtraSize = *(uint16_t*)&Buffer[0];

		return (uint32_t)(30 + FileNameSize + ExtraSize + HeaderOffset);
	}

	void ZipArchive::ReadExtraInfo(uint64_t i, ZipEntry& Entry)
	{
		if (this->_CentralDirLength < 4)
			return;

		uint64_t Pos = i;
		uint32_t Tag, Size;

		uint8_t* Buffer = this->_CentralDir.get();

		while (Pos < this->_CentralDirLength - 4)
		{
			uint32_t ExtraId = *(uint16_t*)(&Buffer[Pos]);
			uint32_t Length = *(uint16_t*)(&Buffer[Pos + 2]);

			if (ExtraId == 0x1)	// Zip64 information
			{
				Tag = *(uint16_t*)(&Buffer[Pos + 8]);
				Size = *(uint16_t*)(&Buffer[Pos + 10]);

				if (Tag == 1 && Size >= 24)
				{
					if (Entry.FileSize == 0xFFFFFFFF)
						Entry.FileSize = *(uint64_t*)(&Buffer[Pos + 12]);
					if (Entry.CompressedSize == 0xFFFFFFFF)
						Entry.CompressedSize = *(uint64_t*)(&Buffer[Pos + 20]);
					if (Entry.HeaderOffset == 0xFFFFFFFF)
						Entry.HeaderOffset = *(uint64_t*)(&Buffer[Pos + 28]);
				}
			}

			Pos += Length + 4;
		}
	}

	string ZipArchive::NormalizeFileName(const string& FileName)
	{
		auto Name = FileName.Replace("\\", "/");

		auto Pos = Name.IndexOf(":");
		if (Pos != string::InvalidPosition)
			Name = Name.Substring(Pos + 1);

		if (Name.Length() > 2 && Name[0] == '/' && Name[Name.Length() - 1] == '/')
			return Name.Substring(1, Name.Length() - 2);
		else if (Name.Length() > 1 && Name[0] == '/')
			return Name.Substring(1);
		else if (Name.Length() > 1 && Name[Name.Length() - 1] == '/')
			return Name.Substring(0, Name.Length() - 1);

		return Name;
	}

	void ZipArchive::UpdateCrcAndSizes(ZipEntry& Entry)
	{
		auto LastPos = this->BaseStream->GetPosition();

		auto Writer = IO::BinaryWriter(this->BaseStream.get(), true);

		this->BaseStream->SetPosition(Entry.HeaderOffset + 8);
		Writer.Write<uint16_t>((uint16_t)Entry.Method);

		this->BaseStream->SetPosition(Entry.HeaderOffset + 14);
		Writer.Write<uint32_t>(Entry.Crc32);
		Writer.Write<uint32_t>(Get32BitSize(Entry.CompressedSize));
		Writer.Write<uint32_t>(Get32BitSize(Entry.FileSize));

		this->BaseStream->SetPosition(LastPos);
	}

	void ZipArchive::WriteLocalHeader(ZipEntry& Entry)
	{
		auto Pos = this->BaseStream->GetPosition();

		auto Writer = IO::BinaryWriter(this->BaseStream.get(), true);

		constexpr uint8_t NoExtraData[] = { 80, 75, 3, 4, 20, 0 };
		constexpr uint8_t NoCrcSize[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

		this->BaseStream->Write((uint8_t*)&NoExtraData[0], 0, sizeof(NoExtraData));

		Writer.Write<uint16_t>((uint16_t)0x0800);		// Encode in UTF8
		Writer.Write<uint16_t>((uint16_t)Entry.Method);
		Writer.Write<uint32_t>(0x0);					// DosTime

		this->BaseStream->Write((uint8_t*)&NoCrcSize[0], 0, sizeof(NoCrcSize));

		Writer.Write<uint16_t>((uint16_t)Entry.FileNameInZip.Length());
		Writer.Write<uint16_t>((uint16_t)72);			// ExtraInfo

		this->BaseStream->Write((uint8_t*)&Entry.FileNameInZip[0], 0, Entry.FileNameInZip.Length());
		
		this->CreateExtraInfo(Entry, this->BaseStream.get());

		Entry.HeaderSize = (uint32_t)(this->BaseStream->GetPosition() - Pos);
	}

	void ZipArchive::CreateExtraInfo(ZipEntry& Entry, IO::Stream* Output)
	{
		uint8_t Buffer[72]{};

		//
		// Zip64 Extended Info
		//

		*(uint16_t*)(&Buffer[0]) = 0x1;		// ZIP64 Tag
		*(uint16_t*)(&Buffer[2]) = 32;		// Length
		*(uint16_t*)(&Buffer[8]) = 1;		// One tag
		*(uint16_t*)(&Buffer[10]) = 24;		// One Size
		*(uint64_t*)(&Buffer[12]) = Entry.FileSize;
		*(uint64_t*)(&Buffer[20]) = Entry.CompressedSize;
		*(uint64_t*)(&Buffer[28]) = Entry.HeaderOffset;

		//
		// NTFS FileTime
		//

		*(uint16_t*)(&Buffer[36]) = 0xA;		// NTFS Tag
		*(uint16_t*)(&Buffer[38]) = 32;			// Length
		*(uint16_t*)(&Buffer[44]) = 1;			// One tag
		*(uint16_t*)(&Buffer[46]) = 24;			// One Size
		*(uint64_t*)(&Buffer[48]) = 0x0;
		*(uint64_t*)(&Buffer[56]) = 0x0;
		*(uint64_t*)(&Buffer[64]) = 0x0;

		Output->Write(Buffer, 0, sizeof(Buffer));
	}

	uint32_t ZipArchive::Get32BitSize(uint64_t Size)
	{
		return Size >= 0xFFFFFFFF ? 0xFFFFFFFF : (uint32_t)Size;
	}
}
