#pragma once

#include <cstdint>
#include "Control.h"
#include "DrawingBase.h"
#include "CancelEventArgs.h"

namespace Forms
{
	// Provides data for the on popup event.
	class PopupEventArgs : public CancelEventArgs
	{
	public:
		PopupEventArgs(Control* Window, Control* Ctrl, bool IsBalloon, Drawing::Size Size);
		~PopupEventArgs() = default;

		// The Associated Window for which the tooltip is being painted.
		Control* AssociatedWindow;
		// The control for which the tooltip is being painted.
		Control* AssociatedControl;

		// Whether the tooltip is Ballooned.
		bool IsBalloon;

		// The rectangle outlining the area in which the painting should be done.
		Drawing::Size ToolTipSize;
	};
}