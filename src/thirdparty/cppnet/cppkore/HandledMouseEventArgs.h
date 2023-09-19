#pragma once

#include <cstdint>
#include "MouseButtons.h"
#include "MouseEventArgs.h"

namespace Forms
{
	// Provides data for all mouse events
	class HandledMouseEventArgs : public MouseEventArgs
	{
	public:
		HandledMouseEventArgs() = default;
		HandledMouseEventArgs(MouseButtons Button, uint32_t Clicks, int32_t X, int32_t Y, int32_t Delta);
		~HandledMouseEventArgs() = default;

		// Whether or not the event was pre handled...
		bool Handled;
	};
}