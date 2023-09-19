#pragma once

#include <cstdint>

namespace Forms
{
	// Specifies the style of control to display.
	enum class FlatStyle
	{
		// The control appears flat.
		Flat,
		// A control appears flat until the mouse pointer
		// moves over
		// it, at which point it appears three-dimensional.
		Popup,
		// The control appears three-dimensional.
		Standard,
		// The control appears three-dimensional.
		System,
	};
}