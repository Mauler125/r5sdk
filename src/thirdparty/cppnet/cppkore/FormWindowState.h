#pragma once

#include <cstdint>

namespace Forms
{
	// Specifies how a form window is displayed.
	enum class FormWindowState
	{
		// A default sized window.
		Normal = 0,
		// A minimized window.
		Minimized = 1,
		// A maximized window.
		Maximized = 2
	};
}