#include "stdafx.h"
#include "TextRenderer.h"

namespace Drawing
{
	void TextRenderer::DrawText(HDC hDC, const string& Text, Font& Font, Rectangle Bounds, Color ForeColor, TextFormatFlags Flags)
	{
		SelectObject(hDC, Font.GetFontHandle());
		SetBkMode(hDC, TRANSPARENT);	// Default text rendering doesn't have a background.
										// just draw text.

		SetTextColor(hDC, ForeColor.ToCOLORREF());

		RECT RcDraw{};
		RcDraw.top = Bounds.Y;
		RcDraw.left = Bounds.X;
		RcDraw.right = Bounds.X + Bounds.Width;
		RcDraw.bottom = Bounds.Y + Bounds.Height;

		DRAWTEXTPARAMS Params{};
		Params.cbSize = sizeof(Params);

		DrawTextExA(hDC, (LPSTR)Text.ToCString(), Text.Length(), &RcDraw, (UINT)Flags, &Params);
	}

	void TextRenderer::DrawText(std::unique_ptr<Drawing::Graphics>& Graphics, const string& Text, Font& Font, Rectangle Bounds, Color ForeColor, TextFormatFlags Flags)
	{
		auto hDC = Graphics->GetHDC();
		DrawText(hDC, Text, Font, Bounds, ForeColor, Flags);
		Graphics->ReleaseHDC(hDC);
	}
}
