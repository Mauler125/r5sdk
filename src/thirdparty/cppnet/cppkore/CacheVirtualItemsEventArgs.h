#pragma once

#include <cstdint>

namespace Forms
{
	// Provides data for the CacheVirtualItems event.
	class CacheVirtualItemsEventArgs
	{
	public:
		CacheVirtualItemsEventArgs(int32_t Start, int32_t End);
		~CacheVirtualItemsEventArgs() = default;

		// The start of the cache index.
		int32_t StartIndex;
		// The end of the cache index.
		int32_t EndIndex;
	};
}