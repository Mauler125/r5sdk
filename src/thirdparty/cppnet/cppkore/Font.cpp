#include "stdafx.h"
#include "Font.h"

namespace Drawing
{
	Font::Font(HWND Handle, const HFONT hFont, const bool OwnsFont)
		: _Handle(Handle), _NativeFont(hFont), _OwnsFont(OwnsFont)
	{
	}

	Font::Font(HWND Handle, const Gdiplus::Font& FontObject)
		: _Handle(Handle), _NativeFont(nullptr), _OwnsFont(true)
	{
		LOGFONTA FontParams{};
		auto hDC = GetDC(Handle);
		auto Gfx = Graphics::FromHDC(hDC);

		FontObject.GetLogFontA(Gfx, &FontParams);

		_NativeFont = CreateFontIndirectA(&FontParams);

		delete Gfx;
		ReleaseDC(Handle, hDC);
	}

	Font::~Font()
	{
		if (_OwnsFont && _NativeFont != nullptr)
			DeleteObject(_NativeFont);

		_NativeFont = nullptr;
		_Handle = nullptr;
	}

	HFONT Font::GetFontHandle()
	{
		return _NativeFont;
	}

	std::unique_ptr<Gdiplus::Font> Font::GetFont()
	{
		auto hDC = GetDC(this->_Handle);
		
		auto Result = std::make_unique<Gdiplus::Font>(hDC, this->_NativeFont);
		ReleaseDC(this->_Handle, hDC);

		return std::move(Result);
	}
}
