#pragma once

#include <cstdint>
#include "StringBase.h"

namespace Diagnostics
{
	// This data structure contains information about a process
	struct ProcessInfo
	{
		uint32_t BasePriority;
		string ProcessName;
		uint32_t ProcessId;
		uint32_t HandleCount;
		uint64_t PoolPagedBytes;
		uint64_t PoolNonPagedBytes;
		uint64_t VirtualBytes;
		uint64_t VirtualBytesPeak;
		uint64_t WorkingSetPeak;
		uint64_t WorkingSet;
		uint64_t PageFileBytesPeak;
		uint64_t PageFileBytes;
		uint64_t PrivateBytes;
		uint32_t SessionId;
	};
}