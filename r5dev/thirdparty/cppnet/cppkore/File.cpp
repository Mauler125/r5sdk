#include "stdafx.h"
#include "File.h"

namespace IO
{
	void File::Copy(const string& SourceFileName, const string& DestinationFileName, bool OverWrite)
	{
		auto Result = CopyFileA((const char*)SourceFileName, (const char*)DestinationFileName, !OverWrite);
		if (!Result)
		{
			auto Error = GetLastError();
			switch (Error)
			{
			case ERROR_FILE_EXISTS:
				IOError::StreamFileExists();
				break;
			case ERROR_ACCESS_DENIED:
				IOError::StreamAccessDenied();
				break;
			}
		}
	}

	void File::Delete(const string& FilePath)
	{
		auto Result = DeleteFileA((const char*)FilePath);
		if (!Result)
		{
			auto Error = GetLastError();
			switch (Error)
			{
			case ERROR_FILE_NOT_FOUND:
				IOError::StreamFileNotFound();
				break;
			case ERROR_ACCESS_DENIED:
				IOError::StreamAccessDenied();
				break;
			}
		}
	}

	void File::Decrypt(const string& FilePath)
	{
		auto Result = DecryptFileA((const char*)FilePath, 0);
		if (!Result)
		{
			auto Error = GetLastError();
			switch (Error)
			{
			case ERROR_FILE_NOT_FOUND:
				IOError::StreamFileNotFound();
				break;
			case ERROR_ACCESS_DENIED:
				IOError::StreamAccessDenied();
				break;
			}
		}
	}

	void File::Encrypt(const string& FilePath)
	{
		auto Result = EncryptFileA((const char*)FilePath);
		if (!Result)
		{
			auto Error = GetLastError();
			switch (Error)
			{
			case ERROR_FILE_NOT_FOUND:
				IOError::StreamFileNotFound();
				break;
			case ERROR_ACCESS_DENIED:
				IOError::StreamAccessDenied();
				break;
			}
		}
	}

	bool File::Exists(const string& FilePath)
	{
		if (FilePath.Length() == 0)
			return false;

		WIN32_FILE_ATTRIBUTE_DATA Data{};
		auto Result = GetFileAttributesExA((const char*)FilePath, GET_FILEEX_INFO_LEVELS::GetFileExInfoStandard, &Data);
		if (Result)
		{
			return (Data.dwFileAttributes != -1) && ((Data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0);
		}

		return false;
	}

	void File::Move(const string& SourceFileName, const string& DestinationFileName, bool OverWrite)
	{
		if (File::Exists(DestinationFileName))
		{
			if (OverWrite)
				File::Delete(DestinationFileName);
			else
				IOError::StreamFileExists();
		}

		auto Result = MoveFileA((const char*)SourceFileName, (const char*)DestinationFileName);
		if (!Result)
		{
			auto Error = GetLastError();
			switch (Error)
			{
			case ERROR_FILE_NOT_FOUND:
				IOError::StreamFileNotFound();
				break;
			case ERROR_ACCESS_DENIED:
				IOError::StreamAccessDenied();
				break;
			}
		}
	}

	std::unique_ptr<FileStream> File::Create(const string& FilePath)
	{
		return std::make_unique<FileStream>(FilePath, FileMode::Create, FileAccess::ReadWrite, FileShare::None);
	}
	
	std::unique_ptr<FileStream> File::Open(const string& FilePath, FileMode Mode)
	{
		return File::Open(FilePath, Mode, (Mode == FileMode::Append ? FileAccess::Write : FileAccess::ReadWrite), FileShare::None);
	}

	std::unique_ptr<FileStream> File::Open(const string& FilePath, FileMode Mode, FileAccess Access)
	{
		return File::Open(FilePath, Mode, Access, FileShare::None);
	}

	std::unique_ptr<FileStream> File::Open(const string& FilePath, FileMode Mode, FileAccess Access, FileShare Share)
	{
		return std::make_unique<FileStream>(FilePath, Mode, Access, Share);
	}

	std::unique_ptr<FileStream> File::OpenRead(const string& FilePath)
	{
		return std::make_unique<FileStream>(FilePath, FileMode::Open, FileAccess::Read, FileShare::Read);
	}

	std::unique_ptr<FileStream> File::OpenWrite(const string& FilePath)
	{
		return std::make_unique<FileStream>(FilePath, FileMode::OpenOrCreate, FileAccess::Write, FileShare::None);
	}

	std::unique_ptr<FileStream> File::OpenAppend(const string & FilePath)
	{
		return std::make_unique<FileStream>(FilePath, FileMode::Append, FileAccess::Write, FileShare::None);
	}

	string File::ReadAllText(const string& FilePath)
	{
		return StreamReader(File::OpenRead(FilePath)).ReadToEnd();
	}

	List<uint8_t> File::ReadAllBytes(const string& FilePath)
	{
		auto FileReader = BinaryReader(File::OpenRead(FilePath));
		auto FileLength = FileReader.GetBaseStream()->GetLength();

		auto Result = List<uint8_t>((uint32_t)FileLength, true);

		FileReader.Read((uint8_t*)Result, 0, FileLength);

		return Result;
	}

	List<string> File::ReadAllLines(const string& FilePath)
	{
		auto Result = List<string>();
		auto FileReader = StreamReader(File::OpenRead(FilePath));
		auto BaseStream = FileReader.GetBaseStream();

		while (!BaseStream->GetIsEndOfFile())
			Result.EmplaceBack(std::move(FileReader.ReadLine()));

		return Result;
	}

	void File::WriteAllText(const string& FilePath, const string& Text)
	{
		StreamWriter(File::Create(FilePath)).Write(Text);
	}

	void File::WriteAllBytes(const string& FilePath, const List<uint8_t>& Bytes)
	{
		BinaryWriter(File::Create(FilePath)).Write((uint8_t*)Bytes, 0, Bytes.Count());
	}

	void File::WriteAllBytes(const string& FilePath, const uint8_t* Bytes, uint64_t Count)
	{
		BinaryWriter(File::Create(FilePath)).Write((uint8_t*)Bytes, 0, Count);
	}

	void File::WriteAllLines(const string& FilePath, const List<string>& Lines)
	{
		auto Writer = StreamWriter(File::Create(FilePath));

		for (auto& Line : Lines)
			Writer.WriteLine(Line);
	}
}