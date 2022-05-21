#include "stdafx.h"
#include "IOError.h"

namespace IO
{
	void IOError::StreamNotOpen()
	{
		throw std::exception("Stream not open");
	}

	void IOError::StreamNoReadSupport()
	{
		throw std::exception("Read not supported");
	}

	void IOError::StreamNoWriteSupport()
	{
		throw std::exception("Write not supported");
	}

	void IOError::StreamNoSeekSupport()
	{
		throw std::exception("Seek not supported");
	}

	void IOError::StreamSetLengthSupport()
	{
		throw std::exception("SetLength is not supported");
	}

	void IOError::StreamBaseStream()
	{
		throw std::exception("The underlying stream was closed");
	}

	void IOError::StreamFileNotFound()
	{
		throw std::exception("The file does not exist");
	}

	void IOError::StreamInUse()
	{
		throw std::exception("The file is in use by another process or thread");
	}

	void IOError::StreamFileExists()
	{
		throw std::exception("The file already exists");
	}

	void IOError::StreamPathInvalid()
	{
		throw std::exception("The file path was invalid");
	}

	void IOError::StreamAccessDenied()
	{
		throw std::exception("File access was denied");
	}

	void IOError::StreamRootMismatch()
	{
		throw std::exception("Source and destination path roots must match");
	}

	void IOError::StreamUnknown()
	{
		throw std::exception("An unknown IO error occured");
	}
}