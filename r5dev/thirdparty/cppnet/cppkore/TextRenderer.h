#pragma once

#include <memory>
#include "DrawingBase.h"
#include "Font.h"
#include "StringBase.h"
#include "TextFormatFlags.h"

#undef DrawText

namespace Drawing
{
	// Provides methods for rendering text using GDI.
	class TextRenderer
	{
		TextRenderer() = delete;
		~TextRenderer() = delete;

	public:

		// Draws the specified text at the specified bounds using the provided color and format flags.
		static void DrawText(HDC hDC, const string& Text, Font& Font, Rectangle Bounds, Color ForeColor, TextFormatFlags Flags = TextFormatFlags::Default);
		// Draws the specified text at the specified bounds using the provided color and format flags.
		static void DrawText(std::unique_ptr<Drawing::Graphics>& Graphics, const string& Text, Font& Font, Rectangle Bounds, Color ForeColor, TextFormatFlags Flags = TextFormatFlags::Default);
	};
}