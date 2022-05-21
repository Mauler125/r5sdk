#pragma once

#include <cstdint>
#include <Windows.h>

namespace Win32
{
	class Win32Error
	{
	public:
		// Sets up a system formatted error code
		static std::exception SystemError(DWORD ErrorCode);
		// Occurs when a subkey is missing
		static std::exception RegSubKeyMissing();
		// Occurs when a subkey is malformed
		static std::exception RegSubKeyMalformed();
		// Occurs when a subkey has nested children
		static std::exception RegSubKeyChildren();
	};
}