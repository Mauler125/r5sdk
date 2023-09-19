#pragma once

#include <cstdint>
#include "StringBase.h"

namespace Diagnostics
{
	// Information about a specific process thread
	struct ProcessThread
	{
		// Returns the unique identifier for the associated thread.
		uint32_t Id;
		// Returns the base priority of the thread which is computed by combining the
		// process priority class with the priority level of the associated thread.
		uint32_t BasePriority;
		// The current priority indicates the actual priority of the associated thread
		uint32_t CurrentPriority;
		// The memory address of the function that was called when the associated thread was started.
		uint64_t StartAddress;
	};
}