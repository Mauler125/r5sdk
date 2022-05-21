#pragma once

#include "Path.h"
#include "IOError.h"
#include "ListBase.h"
#include "StringBase.h"

//
// Remove Win32 macros
//

#undef CreateDirectory
#undef GetCurrentDirectory
#undef SetCurrentDirectory

namespace IO
{
	class Directory
	{
	public:
		// Checks whether or not the specified path exists
		static bool Exists(const String& Path);
		// Creates a new directory at the given path
		static void CreateDirectory(const String& Path);
		// Returns the current directory set
		static String GetCurrentDirectory();
		// Sets the current directory
		static void SetCurrentDirectory(const String& Path);
		// Moves the source path to the destination path
		static void Move(const String& SourcePath, const String& DestinationPath);
		// Copies the source path to the destination path
		static void Copy(const String& SourcePath, const String& DestinationPath, bool OverWrite = false);
		// Deletes a directory, optionally recursive if it's not empty
		static bool Delete(const String& Path, bool Recursive = true);

		// Returns an array of files in the current path
		static List<String> GetFiles(const String& Path);
		// Returns an array of files in the current path matching the search pattern
		static List<String> GetFiles(const String& Path, const String& SearchPattern);
		// Returns an array of folders in the current path
		static List<String> GetDirectories(const String& Path);
		// Returns an array of logical drives
		static List<String> GetLogicalDrives();
	};
}