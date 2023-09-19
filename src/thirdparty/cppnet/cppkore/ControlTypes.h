#pragma once

#include <cstdint>

namespace Forms
{
	// Represents the internal RTTI classes for the Forms controls
	enum class ControlTypes : uint8_t
	{
		Control = 0,
		Button = 1,
		Label = 2,
		CheckBox = 3,
		RadioButton = 4,
		GroupBox = 5,
		Form = 6,
		Panel = 7,
		ListView = 8,
		ComboBox = 9,
		ToolTip = 10,
		TextBox = 11,
	};
}