#pragma once

#include <cstdint>

namespace Forms
{
	// This enumeration represents the possible message box options.
	enum class MessageBoxOptions
	{
		// Specifies that the message box is displayed on the active desktop.
		ServiceNotification = 0x00200000,
		// Specifies that the message box is displayed on the active desktop.
		DefaultDesktopOnly = 0x00020000,
		// Specifies that the message box text is right-aligned.
		RightAlign = 0x00080000,
		// Specifies that the message box text is displayed with Rtl reading order.
		RtlReading = 0x00100000,
	};
}