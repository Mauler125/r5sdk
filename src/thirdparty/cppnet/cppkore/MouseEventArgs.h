#pragma once

#include <cstdint>
#include "MouseButtons.h"

namespace Forms
{
	// Provides data for all mouse events
	class MouseEventArgs
	{
	public:
		MouseEventArgs() = default;
		MouseEventArgs(MouseButtons Button, uint32_t Clicks, int32_t X, int32_t Y, int32_t Delta);
		~MouseEventArgs() = default;

		// The button that was pressed
		MouseButtons Button;
		// The number of clicks
		uint32_t Clicks;
		// The X position of the mouse
		int32_t X;
		// The Y position of the mouse
		int32_t Y;
		// The delta movement of the mouse
		int32_t Delta;
	};
}