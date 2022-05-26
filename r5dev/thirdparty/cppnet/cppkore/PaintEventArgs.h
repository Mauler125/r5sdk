#pragma once

#include <memory>
#include "DrawingBase.h"

namespace Forms
{
	// Provides data for the OnPaint event.
	class PaintEventArgs
	{
	public:
		PaintEventArgs() = default;
		PaintEventArgs(Drawing::Graphics* Graphics, Drawing::Rectangle ClipRectangle);
		PaintEventArgs(HDC Dc, Drawing::Rectangle ClipRectangle);
		virtual ~PaintEventArgs();

		// The graphics object used to paint during this event
		std::unique_ptr<Drawing::Graphics> Graphics;
		// The clipped area used for painting during this event
		Drawing::Rectangle ClipRectangle;

		// Gets the native handle
		HDC NativeHandle();

		// Reset graphics state
		void ResetGraphics();

	private:

		// Internal handle references
		HDC _NativeHandle;
		HPALETTE _OldPalette;
		
		// Internal saved state, only used when working with an HDC
		Drawing::GraphicsState SavedGraphicsState;
	};
}