#include "stdafx.h"
#include "CacheVirtualItemsEventArgs.h"

namespace Forms
{
	CacheVirtualItemsEventArgs::CacheVirtualItemsEventArgs(int32_t Start, int32_t End)
		: StartIndex(Start), EndIndex(End)
	{
	}
}
