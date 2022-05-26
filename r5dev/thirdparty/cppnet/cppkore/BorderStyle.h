#pragma once

#include <cstdint>

namespace Forms
{
	// Specifies the border style for a control or form.
	enum class BorderStyle
	{
		// No border.
		None = 0,
		// A single-line border.
		FixedSingle = 1,
		// A three-dimensional border.
		Fixed3D = 2
	};
}