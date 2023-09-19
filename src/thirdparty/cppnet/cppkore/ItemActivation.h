#pragma once

#include <cstdint>

namespace Forms
{
	// Specifies how the user activates items and the appearance
	// of items as the mouse cursor moves over them.
	enum class ItemActivation
	{
		// Activate items with a double-click.
		// Items do not change appearance.
		Standard = 0,
		// Activate items with a single click. The cursor changes shape and the item
		// text changes color.
		OneClick = 1,
		// Activate items with a
		// double click. The item text changes color.
		TwoClick = 2
	};
}