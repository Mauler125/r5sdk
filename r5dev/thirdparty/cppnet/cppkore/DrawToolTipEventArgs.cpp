#include "stdafx.h"
#include "DrawToolTipEventArgs.h"

namespace Forms
{
	DrawToolTipEventArgs::DrawToolTipEventArgs(HDC Dc, Control* Window, Control* Ctrl, Drawing::Rectangle Bounds, const string& Text, Drawing::Color BackColor, Drawing::Color ForeColor, Drawing::Font* Font)
		: _Dc(Dc), AssociatedWindow(Window), AssociatedControl(Ctrl), Bounds(Bounds), ToolTipText(Text), BackColor(BackColor), ForeColor(ForeColor), Font(Font)
	{
		this->Graphics = std::make_unique<Drawing::Graphics>(Dc);
		this->Graphics->SetPageUnit(Gdiplus::Unit::UnitPixel);
	}

	void DrawToolTipEventArgs::DrawBackground()
	{
		Drawing::SolidBrush Brush(BackColor);
		Graphics->FillRectangle(&Brush, Bounds);
	}

	void DrawToolTipEventArgs::DrawText()
	{
		Drawing::SolidBrush Brush(ForeColor);
		Gdiplus::RectF BoundsF((float)Bounds.X, (float)Bounds.Y, (float)Bounds.Width, (float)Bounds.Height);
		Gdiplus::Font Fnt(_Dc, Font->GetFontHandle());
		Gdiplus::StringFormat Fmt;

		Fmt.SetAlignment(Gdiplus::StringAlignment::StringAlignmentCenter);
		Fmt.SetLineAlignment(Gdiplus::StringAlignment::StringAlignmentCenter);

		auto WideString = ToolTipText.ToWString();

		Graphics->DrawString((wchar_t*)WideString, WideString.Length(), &Fnt, BoundsF, &Fmt, &Brush);
	}

	void DrawToolTipEventArgs::DrawBorder()
	{
		Drawing::SolidBrush Brush(Drawing::GetSystemColor(Drawing::SystemColors::WindowFrame));
		Drawing::Pen Pen(&Brush);

		Drawing::Rectangle BoundsOne(Bounds);
		BoundsOne.Width--;
		BoundsOne.Height--;

		Graphics->DrawRectangle(&Pen, BoundsOne);
	}
}
