#pragma once

#include <cstdint>

namespace Forms
{
	// Specifies the border styles for a form.
	enum class FormBorderStyle
	{
		// No border.
		None = 0,
		// A fixed, single line border.
		FixedSingle = 1,
		// A fixed, three-dimensional border.
		Fixed3D = 2,
		// A thick, fixed dialog-style border.
		FixedDialog = 3,
		// A resizable border.
		Sizable = 4,
		// A tool window border that is not resizable.
		FixedToolWindow = 5,
		// A resizable tool window border.
		SizeableToolWindow = 6
	};
}