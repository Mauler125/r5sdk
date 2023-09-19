#pragma once

#include <cstdint>

namespace Forms
{
	// Specifies how items in
	// a list are sorted.
	enum class SortOrder
	{
		// The items are not sorted.
		None = 0,
		// The items are sorted in ascending order.
		Ascending = 1,
		// The items sorted in descending order.
		Descending = 2
	};
}