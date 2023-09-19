#pragma once

#include <cstdint>

namespace Forms
{
	// Specifies which scroll bars will be visible on a control.
	enum class ScrollBars
	{
		// No scroll bars are shown.
		None = 0,
		// Only horizontal scroll bars are shown.
		Horizontal = 1,
		// Only vertical scroll bars are shown.
		Vertical = 2,
		// Both horizontal and vertical scroll bars are shown.
		Both = 3
	};
}