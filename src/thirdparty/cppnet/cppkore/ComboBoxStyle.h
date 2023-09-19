#pragma once

#include <cstdint>

namespace Forms
{
	// Specifies the ComboBox style.
	enum class ComboBoxStyle
	{
		// The text portion is editable. The list portion is always visible.
		Simple = 0,
		// The text portion is editable. The user must click the arrow button to display the list portion.
		DropDown = 1,
		// The user cannot directly edit the text portion. The user must click the arrow button to display the list portion.
		DropDownList = 2
	};
}