#pragma once

#include <cstdint>

namespace Forms
{
	// Provides data for the OnColumnClick event.
	class ColumnClickEventArgs
	{
	public:
		ColumnClickEventArgs(int32_t Column);
		~ColumnClickEventArgs() = default;

		// The index of the column that was clicked.
		const int32_t Column;
	};
}