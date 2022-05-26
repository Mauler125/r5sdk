#pragma once

#include <cstdint>

namespace Forms
{
	// Specifies how an object or text in a control is
	// horizontally aligned relative to an element of the control.
	enum class HorizontalAlignment
	{
		// The object or text is aligned on the left of the control element.
		Left = 0,
		// The object or text is aligned on the right of the control element.
		Right = 1,
		// The object or text is aligned in the center of the control element.
		Center = 2
	};
}