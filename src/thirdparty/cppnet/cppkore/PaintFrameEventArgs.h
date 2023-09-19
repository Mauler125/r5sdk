#pragma once

#include <cstdint>
#include "PaintEventArgs.h"

namespace Forms
{
	// Provides data for the OnPaintFrame event.
	class PaintFrameEventArgs : public PaintEventArgs
	{
	public:
		PaintFrameEventArgs(HDC Dc, Drawing::Rectangle ClipRectangle, bool Active);
		virtual ~PaintFrameEventArgs() = default;

		// Whether or not the window frame is active.
		bool Active;
	};
}