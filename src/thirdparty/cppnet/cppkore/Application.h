#pragma once

#include <memory>
#include <atomic>
#include "Form.h"

namespace Forms
{
	// Provides static methods and properties to manage an application.
	class Application
	{
	public:
		// Begins running a standard application message loop on the current
		// thread, and makes the specified form visible.
		static void Run(Form* MainWindow, bool DeleteWindow);

		// Begins running a dialog application loop on the
		// current thread, you MUST clean up the dialog after use.
		static void RunDialog(Form* MainDialog);

		// Enables the use of visual style components and initializes GDI+.
		static void EnableVisualStyles();

	private:
		// Runs the main window loop.
		static void RunMainLoop(Form* MainWindow);

		// Whether or not GDI+ has been initialized
		static std::atomic<bool> IsGdipInitialized;
		// The token for GDI+
		static ULONG_PTR GdipToken;

		// Internal cleanup routines
		static void InitializeGdip();
		static void ShutdownGdip();
	};
}