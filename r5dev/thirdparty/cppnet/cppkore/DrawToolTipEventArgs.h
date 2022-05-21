#pragma once

#include <memory>
#include "Control.h"
#include "StringBase.h"
#include "DrawingBase.h"
#include "Font.h"

namespace Forms
{
	// Provides data for the OnDraw event.
	class DrawToolTipEventArgs
	{
	public:
		DrawToolTipEventArgs() = default;
		DrawToolTipEventArgs(HDC Dc, Control* Window, Control* Ctrl, Drawing::Rectangle Bounds, const string& Text, Drawing::Color BackColor, Drawing::Color ForeColor, Drawing::Font* Font);
		~DrawToolTipEventArgs() = default;

		// The graphics object used to paint during this event
		std::unique_ptr<Drawing::Graphics> Graphics;
		// The bounds used for painting during this event
		Drawing::Rectangle Bounds;
		// The text that should be drawn
		string ToolTipText;
		// The font used to draw tooltip text
		Drawing::Font* Font;

		// The control for which the tooltip is being painted.
		Control* AssociatedControl;
		// The window for which the tooltip is being painted.
		Control* AssociatedWindow;

		// Draws the background of the tooltip.
		void DrawBackground();
		// Draws the text of the tooltip.
		void DrawText();
		// Draws the border of the tooltip.
		void DrawBorder();

	private:
		// Internal color states
		Drawing::Color BackColor;
		Drawing::Color ForeColor;

		// Internal DC
		HDC _Dc;
	};
}