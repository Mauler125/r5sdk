#pragma once

#include <cstdint>

namespace Win32
{
	// Registry hive values, built-in hive handles
	enum class RegistryHive : uint32_t
	{
		ClassesRoot = 0x80000000,
		CurrentUser = 0x80000001,
		LocalMachine = 0x80000002,
		Users = 0x80000003,
		PerformanceData = 0x80000004,
		CurrentConfig = 0x80000005,
		DynData = 0x80000006,
	};
}