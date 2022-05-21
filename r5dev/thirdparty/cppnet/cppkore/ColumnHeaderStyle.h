#pragma once

#include <cstdint>

namespace Forms
{
	// Specifies how ListView column headers behave.
	enum class ColumnHeaderStyle
	{
		// No visible column header.
		None = 0,
		// Visible column header that does not respond to clicking.
		NonClickable = 1,
		// Visible column header that responds to clicking.
		Clickable = 2
	};
}