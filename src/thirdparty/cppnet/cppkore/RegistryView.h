#pragma once

#include <cstdint>
#include <Windows.h>

namespace Win32
{
	// Defines which version of the registry we want to view, on 64bit systems with SysWOW64
	enum class RegistryView
	{
		Default = 0,
		Registry64 = KEY_WOW64_64KEY,
		Registry32 = KEY_WOW64_32KEY
	};
}