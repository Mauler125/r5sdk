#pragma once

#include <cstdint>

namespace Forms
{
	// Specifies the initial position of a form.
	enum class FormStartPosition
	{
		// The location and size of the form will determine its
		// starting position.
		Manual = 0,
		// The form is centered on the current display,
		// and has the dimensions specified in the form's size.
		CenterScreen = 1,
		// The form is positioned at the Windows default
		// location and has the dimensions specified in the form's size.
		WindowsDefaultLocation = 2,
		// The form is positioned at the Windows default
		// location and has the bounds determined by Windows default.
		WindowsDefaultBounds = 3,
		// The form is centered within the bounds of its parent form.
		CenterParent = 4
	};
}