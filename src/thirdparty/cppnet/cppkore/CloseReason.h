#pragma once

#include <cstdint>

namespace Forms
{
	// Specifies the reason for the Form Closing.
	enum class CloseReason
	{
		// No reason for closure of the Form.
		None = 0,
		// In the process of shutting down, Windows has closed the application.
		WindowsShutDown = 1,
		// The parent form of this MDI form is closing.
		MdiFormClosing = 2,
		// The user has clicked the close button on the form window, selected Close from the window's control menu or hit Alt + F4
		UserClosing = 3,
		// The Microsoft Windows Task Manager is closing the application.
		TaskManagerClosing = 4,
		// A form is closing because its owner is closing.
		FormOwnerClosing = 5,
		// A form is closing because Application.Exit() was called.
		ApplicationExitCall = 6
	};
}