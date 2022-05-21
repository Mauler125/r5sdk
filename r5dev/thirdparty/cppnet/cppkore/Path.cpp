#include "stdafx.h"
#include "Path.h"

namespace IO
{
	String Path::ChangeExtension(const String& FilePath, const String& Extension)
	{
		String Base;
		for (int32_t i = FilePath.Length(); --i >= 0;)
		{
			auto& Ch = FilePath[i];
			if (Ch == '.')
			{
				Base = FilePath.SubString(0, i);
				break;
			}

			// Ignore the separators, we made it too far
			if (Ch == DirectorySeparatorChar || Ch == AltDirectorySeparatorChar || Ch == VolumeSeparatorChar)
				break;
		}

		if (Base.Length() > 0)
		{
			if (Extension.Length() == 0 || Extension[0] != '.')
			{
				Base += ".";
				Base += Extension;
			}
			else
			{
				Base += Extension;
			}
		}

		return Base;
	}

	String Path::GetDirectoryName(const String& Path)
	{
		// Cache the full length and root length
		int32_t cLength = (int32_t)Path.Length();
		int32_t rLength = (cLength > 2 && Path[1] == VolumeSeparatorChar && (Path[2] == DirectorySeparatorChar || Path[2] == AltDirectorySeparatorChar)) ? 3 : 0;

		if (cLength > rLength)
		{
			// Iterate while not modifying the String for performance
			while (cLength > rLength && Path[--cLength] != DirectorySeparatorChar && Path[cLength] != AltDirectorySeparatorChar);

			return Path.SubString(0, cLength);
		}

		return "";
	}

	String Path::GetExtension(const String& FilePath)
	{
		// Cache full length
		int32_t cLength = (int32_t)FilePath.Length();

		for (int32_t i = cLength; --i >= 0;)
		{
			auto& Ch = FilePath[i];
			if (Ch == '.')
			{
				if (i != cLength - 1)
					return FilePath.SubString(i, cLength - i);
				else
					return "";
			}

			// If we found one of these, we made it too far
			if (Ch == DirectorySeparatorChar || Ch == AltDirectorySeparatorChar || Ch == VolumeSeparatorChar)
				break;
		}

		return "";
	}

	String Path::GetFileName(const String& FilePath)
	{
		int32_t cLength = (int32_t)FilePath.Length();
		for (int32_t i = cLength; --i >= 0;)
		{
			auto& Ch = FilePath[i];

			if (Ch == DirectorySeparatorChar || Ch == AltDirectorySeparatorChar || Ch == VolumeSeparatorChar)
				return FilePath.SubString(i + 1, cLength - i - 1);
		}

		return FilePath;
	}

	String Path::GetFileNameWithoutExtension(const String& FilePath)
	{
		auto fPath = Path::GetFileName(FilePath);
		auto fExt = fPath.LastIndexOf('.');

		if (fExt != String::InvalidPosition)
			return fPath.SubString(0, fExt);
		else
			return fPath;
	}

	String Path::GetPathRoot(const String& Path)
	{
		return Path.SubString(0, Path::GetRootLength(Path));
	}

	String Path::GetTempPath()
	{
		char Buffer[MAX_PATH + 1]{};
		GetTempPathA(MAX_PATH, Buffer);

		return Buffer;
	}

	String Path::GetTempFileName()
	{
		auto BasePath = Path::GetTempPath();

		char Buffer[MAX_PATH + 1]{};
		GetTempFileNameA((const char*)BasePath, "tmp", 0, (char*)Buffer);

		return Buffer;
	}

	bool Path::HasExtension(const String& FilePath)
	{
		auto cLength = FilePath.Length();

		for (int32_t i = cLength; --i >= 0;)
		{
			auto& Ch = FilePath[i];
			if (Ch == '.')
			{
				if (i != cLength - 1)
					return true;
				else
					return false;
			}

			// If we found one of these, we made it too far
			if (Ch == DirectorySeparatorChar || Ch == AltDirectorySeparatorChar || Ch == VolumeSeparatorChar)
				break;
		}

		return false;
	}

	bool Path::IsPathRooted(const String& Path)
	{
		auto cLength = Path.Length();
		if ((cLength >= 1 && (Path[0] == DirectorySeparatorChar || Path[0] == AltDirectorySeparatorChar)) || (cLength >= 2 && Path[1] == VolumeSeparatorChar))
			return true;

		return false;
	}

	String Path::Combine(const String& Path1, const String& Path2)
	{
		if (Path2.Length() == 0)
			return Path1;
		if (Path1.Length() == 0)
			return Path2;

		if (Path::IsPathRooted(Path2))
			return Path2;

		auto& Ch = Path1[Path1.Length() - 1];
		if (Ch != DirectorySeparatorChar && Ch != AltDirectorySeparatorChar && Ch != VolumeSeparatorChar)
			return Path1 + DirectorySeparatorChar + Path2;

		return Path1 + Path2;
	}

	uint32_t Path::GetRootLength(const String& Path)
	{
		int32_t i = 0, cLength = Path.Length();

		auto IsDirectorySeparator = [](char Ch) -> bool
		{
			return (Ch == DirectorySeparatorChar || Ch == AltDirectorySeparatorChar);
		};

		if (cLength >= 1 && IsDirectorySeparator(Path[0]))
		{
			// Handles UNC paths and currrent drive root paths
			i = 1;
			if (cLength >= 2 && IsDirectorySeparator(Path[1]))
			{
				i = 2;
				int32_t nIter = 2;

				while (i < cLength && !IsDirectorySeparator(Path[i]) || --nIter > 0)
					i++;
			}
		}
		else if (cLength >= 2 && Path[1] == VolumeSeparatorChar)
		{
			// Handles C:\Windows
			i = 2;
			if (cLength >= 3 && IsDirectorySeparator(Path[2]))
				i++;
		}

		return i;
	}
}