#pragma once

#include <cstdint>

namespace Forms
{
	// This enumeration represents the possible default message box buttons.
	enum class MessageBoxDefaultButton
	{
		// Specifies that the first button on the message box should be the default button.
		Button1 = 0x00000000,
		// Specifies that the second button on the message box should be the default button.
		Button2 = 0x00000100,
		// Specifies that the third button on the message box should be the default button.
		Button3 = 0x00000200,
	};
}