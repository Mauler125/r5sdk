#pragma once

namespace IO
{
	enum class FileShare
	{
		// No sharing. Any request to open the file (by this process or another
		// process) will fail until the file is closed.
		None = 0,
		// Allows subsequent opening of the file for reading. If this flag is not
		// specified, any request to open the file for reading (by this process or
		// another process) will fail until the file is closed.
		Read = 1,
		// Allows subsequent opening of the file for writing. If this flag is not
		// specified, any request to open the file for writing (by this process or
		// another process) will fail until the file is closed.
		Write = 2,
		// Allows subsequent opening of the file for writing or reading. If this flag
		// is not specified, any request to open the file for writing or reading (by
		// this process or another process) will fail until the file is closed.
		ReadWrite = 3,
		// Open the file, but allow someone else to delete the file.
		Delete = 4,
	};
}