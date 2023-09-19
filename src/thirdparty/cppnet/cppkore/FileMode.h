#pragma once

namespace IO
{
	enum class FileMode
	{
		// Creates a new file. An exception is raised if the file already exists.
		CreateNew = 1,
		// Creates a new file. If the file already exists, it is overwritten.
		Create = 2,
		// Opens an existing file. An exception is raised if the file does not exist.
		Open = 3,
		// Opens the file if it exists. Otherwise, creates a new file.
		OpenOrCreate = 4,
		// Opens an existing file. Once opened, the file is truncated so that its
		// size is zero bytes. The calling process must open the file with at least
		// WRITE access. An exception is raised if the file does not exist.
		Truncate = 5,
		// Opens the file if it exists and seeks to the end.  Otherwise, 
		// creates a new file.
		Append = 6
	};
}