#pragma once

#include <memory>
#include "DrawingBase.h"

namespace Drawing
{
	// Represents a font used for text rendering
	class Font
	{
	public:
		Font() = default;
		Font(HWND Handle, const HFONT hFont, const bool OwnsFont = false);
		Font(HWND Handle, const Gdiplus::Font& FontObject);

		virtual ~Font();

		// Gets a handle to the native font
		HFONT GetFontHandle();
		// Returns a GDI+ font
		std::unique_ptr<Gdiplus::Font> GetFont();

	private:
		// Internal handle
		HWND _Handle;
		// Internal native handle
		HFONT _NativeFont;
		// Internal cached state
		bool _OwnsFont;
	};
}