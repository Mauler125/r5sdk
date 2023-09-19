#pragma once

#include <array>
#include "StringBase.h"

//
// Remove Win32 macros
//

#undef GetTempPath
#undef GetTempFileName

namespace IO
{
	class Path
	{
	public:
		// Platform specific directory separator character
		constexpr static char DirectorySeparatorChar = '\\';
		// Alternate platform specific directory separator character
		constexpr static char AltDirectorySeparatorChar = '/';
		// Platform specific volume separatoer character
		constexpr static char VolumeSeparatorChar = ':';

		// Changes the extension of a file path
		static String ChangeExtension(const String& FilePath, const String& Extension);
		// Returns the directory path of a file path
		static String GetDirectoryName(const String& Path);
		// Returns the extension of the given path
		static String GetExtension(const String& FilePath);
		// Returns the name and extension parts of the given path
		static String GetFileName(const String& FilePath);
		// Returns the name without the extension of the given path
		static String GetFileNameWithoutExtension(const String& FilePath);
		// Returns the root portion of the given path
		static String GetPathRoot(const String& Path);
		// Returns a temporary folder path
		static String GetTempPath();
		// Returns a temporary file name
		static String GetTempFileName();
		// Checks if the file name has an extension
		static bool HasExtension(const String& FilePath);
		// Checks if the given path contains a root
		static bool IsPathRooted(const String& Path);
		// Combine two paths
		static String Combine(const String& Path1, const String& Path2);

		// Returns a list of all the bad path characters
		constexpr static std::array<char, 36> GetInvalidPathChars()
		{
			constexpr std::array<char, 36> Buffer =
			{
				'\"', '<', '>', '|', '\0',
				(char)1, (char)2, (char)3, (char)4, (char)5, (char)6, (char)7, (char)8, (char)9, (char)10,
				(char)11, (char)12, (char)13, (char)14, (char)15, (char)16, (char)17, (char)18, (char)19, (char)20,
				(char)21, (char)22, (char)23, (char)24, (char)25, (char)26, (char)27, (char)28, (char)29, (char)30,
				(char)31
			};

			return Buffer;
		}

		// Returns a list of all the bad file name characters
		constexpr static std::array<char, 41> GetInvalidFileNameChars()
		{
			constexpr std::array<char, 41> Buffer =
			{
				'\"', '<', '>', '|', '\0',
				(char)1, (char)2, (char)3, (char)4, (char)5, (char)6, (char)7, (char)8, (char)9, (char)10,
				(char)11, (char)12, (char)13, (char)14, (char)15, (char)16, (char)17, (char)18, (char)19, (char)20,
				(char)21, (char)22, (char)23, (char)24, (char)25, (char)26, (char)27, (char)28, (char)29, (char)30,
				(char)31, ':', '*', '?', '\\', '/'
			};

			return Buffer;
		}

	private:

		//
		// Internal helper routines
		//

		static uint32_t GetRootLength(const String& Path);
	};
}