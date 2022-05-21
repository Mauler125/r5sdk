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
		static bool Exists(const string& Path);
		// Creates a new directory at the given path
		static void CreateDirectory(const string& Path);
		// Returns the current directory set
		static string GetCurrentDirectory();
		// Sets the current directory
		static void SetCurrentDirectory(const string& Path);
		// Moves the source path to the destination path
		static void Move(const string& SourcePath, const string& DestinationPath);
		// Copies the source path to the destination path
		static void Copy(const string& SourcePath, const string& DestinationPath, bool OverWrite = false);
		// Deletes a directory, optionally recursive if it's not empty
		static bool Delete(const string& Path, bool Recursive = true);

		// Returns an array of files in the current path
		static List<string> GetFiles(const string& Path);
		// Returns an array of files in the current path matching the search pattern
		static List<string> GetFiles(const string& Path, const string& SearchPattern);
		// Returns an array of folders in the current path
		static List<string> GetDirectories(const string& Path);
		// Returns an array of logical drives
		static List<string> GetLogicalDrives();
	};
}