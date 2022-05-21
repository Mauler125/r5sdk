#pragma once

namespace IO
{
	class IOError
	{
	public:
		// Occurs when the stream is closed
		static void StreamNotOpen();
		// Occurs when reading is not supported
		static void StreamNoReadSupport();
		// Occurs when writing is not supported
		static void StreamNoWriteSupport();
		// Occurs when seek is not supported
		static void StreamNoSeekSupport();
		// Occurs when setlength is not supported
		static void StreamSetLengthSupport();
		// Occurs when the basestream was closed
		static void StreamBaseStream();
		// Occurs when the file doesn't exist
		static void StreamFileNotFound();
		// Occurs when the file is in use by another process
		static void StreamInUse();
		// Occurs when a file already exists
		static void StreamFileExists();
		// Occurs when the input path is invalid
		static void StreamPathInvalid();
		// Occurs when we don't have enouch permissions
		static void StreamAccessDenied();
		// Occurs when the roots of a move operation don't match
		static void StreamRootMismatch();
		// Occurs during an unknown stream error
		static void StreamUnknown();
	};
}