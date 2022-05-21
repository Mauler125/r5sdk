#pragma once

#include <cstdint>

namespace Forms
{
	// Specifies the state of a control,
	// such as a check box, that can be checked, unchecked, or
	// set to an indeterminate state.
	enum class CheckState
	{
		// The control is unchecked.
		Unchecked = 0,
		// The control is checked.
		Checked = 1,
		// The control is indeterminate. An indeterminate control generally has a shaded appearance.
		Indeterminate = 2
	};
}