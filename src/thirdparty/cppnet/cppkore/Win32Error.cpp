#include "stdafx.h"
#include "Win32Error.h"

namespace Win32
{
	std::exception Win32Error::SystemError(DWORD ErrorCode)
	{
		char MessageBuffer[1024];
		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, ErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), MessageBuffer, 1024, NULL);

		return std::exception(MessageBuffer);
	}

	std::exception Win32Error::RegSubKeyMissing()
	{
		return std::exception("The subkey was not found");
	}

	std::exception Win32Error::RegSubKeyMalformed()
	{
		return std::exception("Malformed subkey detected");
	}

	std::exception Win32Error::RegSubKeyChildren()
	{
		return std::exception("The subkey contains nested children");
	}
}
