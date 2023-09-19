#pragma once

#include <cstdint>

namespace Forms
{
	// Specifies responsibility for drawing a control or portion of a control.
	enum class DrawMode
	{
		// The operating system paints the items in the control, and the items are each the same height.
		Normal = 0,
		// The programmer explicitly paints the items in the control, and the items are the same height.
		OwnerDrawFixed = 1,
		// The programmer explicitly paints the items in the control manually, and they may be different heights.
		OwnerDrawVariable = 2
	};
}