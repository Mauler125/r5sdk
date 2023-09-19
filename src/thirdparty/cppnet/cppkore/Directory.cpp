#include "stdafx.h"
#include "Directory.h"

namespace IO
{
	void Directory::CreateDirectory(const String& Path)
	{
		if (Path.Length() == 0)
			return;
		if (Directory::Exists(Path))
			return;

		int32_t fLength = Path.Length();
		int32_t rLength = (fLength > 2 && Path[1] == Path::VolumeSeparatorChar && (Path[2] == Path::DirectorySeparatorChar || Path[2] == Path::AltDirectorySeparatorChar)) ? 3 : 0;

		// We need to trim the trailing slash or the code will try to create 2 directories of the same name
		if (fLength >= 2 && (Path[fLength - 1] == Path::DirectorySeparatorChar || Path[fLength - 1] == Path::AltDirectorySeparatorChar))
			fLength--;

		// A list of directories to make
		auto DirectoryStack = List<String>();

		if (fLength > rLength)
		{
			int32_t i = fLength - 1;
			while (i >= rLength)
			{
				DirectoryStack.EmplaceBack(std::move(Path.SubString(0, i + 1)));

				while (i > rLength && Path[i] != Path::DirectorySeparatorChar && Path[1] != Path::AltDirectorySeparatorChar)
					i--;

				i--;
			}
		}

		// Iteration in stack-based order FILO
		for (int32_t i = DirectoryStack.Count() - 1; i >= 0; --i)
			CreateDirectoryA((const char*)DirectoryStack[i], NULL);
	}

	bool Directory::Exists(const String& Path)
	{
		if (Path.Length() == 0)
			return false;

		WIN32_FILE_ATTRIBUTE_DATA Data{};
		auto Result = GetFileAttributesExA((const char*)Path, GET_FILEEX_INFO_LEVELS::GetFileExInfoStandard, &Data);
		if (Result)
		{
			return (Data.dwFileAttributes != -1) && ((Data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
		}

		return false;
	}

	List<String> Directory::GetFiles(const String& Path)
	{
		return Directory::GetFiles(Path, "*");
	}

	List<String> Directory::GetFiles(const String& Path, const String& SearchPattern)
	{
		auto Result = List<String>();

		auto sQuery = (Path[Path.Length() - 1] == Path::DirectorySeparatorChar || Path[Path.Length() - 1] == Path::AltDirectorySeparatorChar) ? Path + SearchPattern : Path + Path::DirectorySeparatorChar + SearchPattern;

		WIN32_FIND_DATAA fInfo;
		auto ResultHandle = FindFirstFileA((const char*)sQuery, &fInfo);

		if (ResultHandle == INVALID_HANDLE_VALUE)
			return Result;

		if (_strnicmp(fInfo.cFileName, ".", 1) != 0 && _strnicmp(fInfo.cFileName, "..", 2) != 0)
		{
			auto isDir = (0 != (fInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY));
			if (!isDir)
				Result.EmplaceBack(std::move(Path::Combine(Path, fInfo.cFileName)));
		}

		while (FindNextFileA(ResultHandle, &fInfo))
		{
			if (_strnicmp(fInfo.cFileName, ".", 1) != 0 && _strnicmp(fInfo.cFileName, "..", 2) != 0)
			{
				auto isDir = (0 != (fInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY));
				if (!isDir)
					Result.EmplaceBack(std::move(Path::Combine(Path, fInfo.cFileName)));
			}
		}

		FindClose(ResultHandle);

		return Result;
	}

	List<String> Directory::GetDirectories(const String& Path)
	{
		auto Result = List<String>();

		auto sQuery = (Path[Path.Length() - 1] == Path::DirectorySeparatorChar || Path[Path.Length() - 1] == Path::AltDirectorySeparatorChar) ? Path + "*" : Path + Path::DirectorySeparatorChar + "*";

		WIN32_FIND_DATAA fInfo;
		auto ResultHandle = FindFirstFileA((const char*)sQuery, &fInfo);

		if (ResultHandle == INVALID_HANDLE_VALUE)
			return Result;

		if (_strnicmp(fInfo.cFileName, ".", 1) != 0 && _strnicmp(fInfo.cFileName, "..", 2) != 0)
		{
			auto isDir = (0 != (fInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY));
			if (isDir)
				Result.EmplaceBack(std::move(Path::Combine(Path, fInfo.cFileName)));
		}

		while (FindNextFileA(ResultHandle, &fInfo))
		{
			if (_strnicmp(fInfo.cFileName, ".", 1) != 0 && _strnicmp(fInfo.cFileName, "..", 2) != 0)
			{
				auto isDir = (0 != (fInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY));
				if (isDir)
					Result.EmplaceBack(std::move(Path::Combine(Path, fInfo.cFileName)));
			}
		}

		FindClose(ResultHandle);

		return Result;
	}

	List<String> Directory::GetLogicalDrives()
	{
		auto Result = List<String>();

		auto dCount = ::GetLogicalDrives();
		if (dCount == 0)
			IOError::StreamAccessDenied();

		uint32_t d = (uint32_t)dCount;
		char Root[] = { 'A', ':', '\\' };

		while (d != 0)
		{
			if ((d & 1) != 0)
				Result.EmplaceBack(std::move(String(Root, 3)));

			d >>= 1;
			Root[0]++;
		}

		return Result;
	}

	String Directory::GetCurrentDirectory()
	{
		char Buffer[MAX_PATH + 1]{};
		if (!GetCurrentDirectoryA(MAX_PATH, (char*)Buffer))
		{
			auto Error = GetLastError();
			switch (Error)
			{
			case ERROR_ACCESS_DENIED:
				IOError::StreamAccessDenied();
				break;
			default:
				IOError::StreamUnknown();
				break;
			}
		}

		return Buffer;
	}

	void Directory::SetCurrentDirectory(const String& Path)
	{
		auto Result = SetCurrentDirectoryA((const char*)Path);
		if (!Result)
		{
			auto Error = GetLastError();
			switch (Error)
			{
			case ERROR_ACCESS_DENIED:
				IOError::StreamAccessDenied();
				break;
			case ERROR_FILE_NOT_FOUND:
			case ERROR_PATH_NOT_FOUND:
				IOError::StreamFileNotFound();
				break;
			default:
				IOError::StreamUnknown();
				break;
			}
		}
	}

	void Directory::Move(const String& SourcePath, const String& DestinationPath)
	{
		// Ensure that the roots are the same
		if (Path::GetPathRoot(SourcePath).ToLower() != Path::GetPathRoot(DestinationPath).ToLower())
			IOError::StreamRootMismatch();

		auto Result = MoveFileA((const char*)SourcePath, (const char*)DestinationPath);
		if (!Result)
		{
			auto Error = GetLastError();
			switch (Error)
			{
			case ERROR_ACCESS_DENIED:
				IOError::StreamAccessDenied();
				break;
			case ERROR_FILE_NOT_FOUND:
			case ERROR_PATH_NOT_FOUND:
				IOError::StreamFileNotFound();
				break;
			default:
				IOError::StreamUnknown();
				break;
			}
		}
	}

	void Directory::Copy(const String& SourcePath, const String& DestinationPath, bool OverWrite)
	{
		auto Result = CopyFileA((const char*)SourcePath, (const char*)DestinationPath, !OverWrite);
		if (!Result)
		{
			auto Error = GetLastError();
			switch (Error)
			{
			case ERROR_ACCESS_DENIED:
				IOError::StreamAccessDenied();
				break;
			case ERROR_FILE_NOT_FOUND:
			case ERROR_PATH_NOT_FOUND:
				IOError::StreamFileNotFound();
				break;
			case ERROR_FILE_EXISTS:
				IOError::StreamFileExists();
				break;
			default:
				IOError::StreamUnknown();
				break;
			}
		}
	}

	bool Directory::Delete(const String& Path, bool Recursive)
	{
		if (Path.Length() == 0)
			return false;
		if (!Directory::Exists(Path))
			return false;

		if (!Recursive)
			return (RemoveDirectoryA((const char*)Path) != 0);

		//
		// We must recursively delete each file and folder in the path, because RemoveDirectory doesn't handle non-empty file paths
		// If one of the operations fails, the entire delete operation fails
		//

		auto sQuery = (Path[Path.Length() - 1] == Path::DirectorySeparatorChar || Path[Path.Length() - 1] == Path::AltDirectorySeparatorChar) ? Path + "*" : Path + Path::DirectorySeparatorChar + "*";

		WIN32_FIND_DATAA fInfo;
		auto ResultHandle = FindFirstFileA((const char*)sQuery, &fInfo);

		if (ResultHandle == INVALID_HANDLE_VALUE)
			return false;

		do
		{
			auto bDir = (0 != (fInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY));
			if (bDir)
			{
				auto bReparse = (0 != (fInfo.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT));
				if (!bReparse)
				{
					return Directory::Delete(Path::Combine(Path, fInfo.cFileName), true);
				}
				else if (bReparse && fInfo.dwReserved0 == IO_REPARSE_TAG_MOUNT_POINT)
				{
					if (!DeleteVolumeMountPointA((const char*)Path::Combine(Path, String(fInfo.cFileName) + Path::DirectorySeparatorChar)))
						return false;
				}
			}
			else
			{
				if (!DeleteFileA((const char*)Path::Combine(Path, fInfo.cFileName)))
					return false;
			}
		} while (FindNextFileA(ResultHandle, &fInfo));

		FindClose(ResultHandle);

		if (RemoveDirectoryA((const char*)Path))
			return true;

		return false;
	}
}