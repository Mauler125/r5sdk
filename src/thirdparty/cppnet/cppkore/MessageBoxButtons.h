#pragma once

#include <cstdint>

namespace Forms
{
	// This enumeration represents the possible button options for a message box.
	enum class MessageBoxButtons
	{
		// Specifies that the message box contains an OK button.
		OK = 0x00000000,
		// Specifies that the message box contains OK and Cancel buttons.
		OKCancel = 0x00000001,
		// Specifies that the message box contains Abort, Retry, and Ignore buttons.
		AbortRetryIgnore = 0x00000002,
		// Specifies that the message box contains Yes, No, and Cancel buttons.
		YesNoCancel = 0x00000003,
		// Specifies that the message box contains Yes and No buttons.
		YesNo = 0x00000004,
		// Specifies that the message box contains Retry and Cancel buttons.
		RetryCancel = 0x00000005,
	};
}