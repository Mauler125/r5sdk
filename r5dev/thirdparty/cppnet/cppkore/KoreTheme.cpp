#include "stdafx.h"
#include "KoreTheme.h"
#include "MessageBox.h"
#include "TextRenderer.h"
#include "CheckBoxImage.h"

namespace Themes
{
	// Constants for brushes
	/*const static auto BorderBrush = Drawing::Color(219, 56, 80);
	const static auto DisabledBorderBrush = Drawing::Color(30, 32, 55);


	const static auto BackgroundBrush = Drawing::Color(30, 32, 55);
	const static auto BackgroundLightBrush = Drawing::Color(31, 37, 62);


	const static auto BackgroundGrad1 = Drawing::Color(30, 32, 55);
	const static auto BackgroundGrad2 = Drawing::Color(30, 32, 55);


	const static auto BackgroundOverGrad1 = Drawing::Color(30, 32, 55);
	const static auto BackgroundOverGrad2 = Drawing::Color(30, 32, 55);


	const static auto TextEnabledBrush = Drawing::Color(Drawing::Color::White);
	const static auto TextDisabledBrush = Drawing::Color(Drawing::Color::Gray);


	const static auto ProgressGrad1 = Drawing::Color(219, 56, 80);
	const static auto ProgressGrad2 = Drawing::Color(219, 56, 80);

	const static auto HeaderBrush = Drawing::Color(46, 53, 84);*/

	// New Theme, saving for anyone else who wants to help.
	const static auto BorderBrush = Drawing::Color(113, 156, 235);
	const static auto DarkBorderBrush = Drawing::Color(113, 156, 235);
	const static auto DisabledBorderBrush = Drawing::Color(160, 160, 160);

	const static auto BackgroundBrush = Drawing::Color(33, 33, 33);
	const static auto BackgroundLightBrush = Drawing::Color(39, 39, 39);

	const static auto BackgroundGrad1 = Drawing::Color(45, 45, 45);
	const static auto BackgroundGrad2 = Drawing::Color(36, 36, 36);

	const static auto BackgroundOverGrad1 = Drawing::Color(49, 49, 49);
	const static auto BackgroundOverGrad2 = Drawing::Color(40, 40, 40);

	const static auto TextEnabledBrush = Drawing::Color(Drawing::Color::White);
	const static auto TextDisabledBrush = Drawing::Color(Drawing::Color::Gray);

	const static auto ProgressGrad1 = Drawing::Color(113, 156, 235);
	const static auto ProgressGrad2 = Drawing::Color(113, 156, 235);

	const static auto HeaderBrush = Drawing::Color(54, 54, 54);
	
	// Constants for images
	static Drawing::Image* CheckBoxImage = nullptr;

	KoreTheme::KoreTheme()
		: UIX::UIXRenderer()
	{
		CheckBoxImage = Drawing::ImageFromTgaData(CheckBoxImage_Src, sizeof(CheckBoxImage_Src));

		// Change the message box colors to represent our theme
		Forms::MessageBox::SetMessageBoxColors(Drawing::Color::White, BackgroundBrush, BackgroundLightBrush);
	}

	KoreTheme::~KoreTheme()
	{
		delete CheckBoxImage;
	}

	void KoreTheme::RenderControlBorder(const std::unique_ptr<Forms::PaintEventArgs>& EventArgs, Forms::Control* Ctrl, UIX::UIXRenderState State) const
	{
		Drawing::Rectangle Rect(Ctrl->ClientRectangle());

		//
		// Override for textbox rendering due to a bug in the layout rect
		//

		if (Ctrl->GetType() == Forms::ControlTypes::TextBox)
		{
			Rect.Width = Ctrl->Size().Width;
			Rect.Height = Ctrl->Size().Height;
		}

		Rect.Width--;
		Rect.Height--;

		Drawing::SolidBrush Brush((State == UIX::UIXRenderState::Disabled) ? DisabledBorderBrush : BorderBrush);
		Drawing::Pen Pen(&Brush);
		
		// Check again for textbox due to size issues
		if (Ctrl->GetType() == Forms::ControlTypes::TextBox)
		{
			auto brush = Drawing::SolidBrush(BackgroundBrush);
			auto pen = Drawing::Pen(&brush);
			EventArgs->Graphics->DrawRectangle(&pen, Rect);
		}
		
		// Now render smooth border
		auto SmMode = EventArgs->Graphics->GetSmoothingMode();
		{

			EventArgs->Graphics->SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeAntiAlias);
			Drawing::DrawRoundRectangle(EventArgs->Graphics.get(), &Pen, Rect, 2);
		}
		EventArgs->Graphics->SetSmoothingMode(SmMode);
	}

	void KoreTheme::RenderControlBackground(const std::unique_ptr<Forms::PaintEventArgs>& EventArgs, Forms::Control* Ctrl, UIX::UIXRenderState State) const
	{
		Drawing::Rectangle Rect(Ctrl->ClientRectangle());

		auto brush = Drawing::SolidBrush(BackgroundBrush);
		EventArgs->Graphics->FillRectangle(&brush, Rect);
	}

	void KoreTheme::RenderControlButtonBackground(const std::unique_ptr<Forms::PaintEventArgs>& EventArgs, Forms::Control* Ctrl, UIX::UIXRenderState State) const
	{
		Drawing::Rectangle Rect(Ctrl->ClientRectangle());

		std::unique_ptr<Drawing::Brush> DrawBrush = nullptr;

		switch (State)
		{
		case UIX::UIXRenderState::Default:
			DrawBrush = std::make_unique<Drawing::LinearGradientBrush>(Rect, BackgroundGrad1, BackgroundGrad2, 90.f);
			break;
		case UIX::UIXRenderState::Disabled:
			DrawBrush = std::make_unique<Drawing::LinearGradientBrush>(Rect, BackgroundGrad1, BackgroundGrad2, 90.f);
			break;
		case UIX::UIXRenderState::MouseOver:
			DrawBrush = std::make_unique<Drawing::LinearGradientBrush>(Rect, BackgroundOverGrad1, BackgroundOverGrad2, 90.f);
			break;
		case UIX::UIXRenderState::MouseDown:
			DrawBrush = std::make_unique<Drawing::LinearGradientBrush>(Rect, BackgroundGrad1, BackgroundGrad2, 270.f);
			break;
		}

		EventArgs->Graphics->FillRectangle(DrawBrush.get(), Rect);
	}

	void KoreTheme::RenderControlText(const std::unique_ptr<Forms::PaintEventArgs>& EventArgs, Forms::Control* Ctrl, UIX::UIXRenderState State, Drawing::Rectangle LayoutRect, Drawing::ContentAlignment Alignment) const
	{
		// Calculate formatting flags
		auto FormatFlags = Drawing::TextFormatFlags::WordBreak;

		// Handle horizontal alignment
		if (((int)Alignment & (int)Drawing::AnyLeftAlign) != 0)
			FormatFlags |= Drawing::TextFormatFlags::Left;
		else if (((int)Alignment & (int)Drawing::AnyRightAlign) != 0)
			FormatFlags |= Drawing::TextFormatFlags::Right;
		else
			FormatFlags |= Drawing::TextFormatFlags::HorizontalCenter;

		// Handle vertical alignment
		if (((int)Alignment & (int)Drawing::AnyTopAlign) != 0)
			FormatFlags |= Drawing::TextFormatFlags::Top;
		else if (((int)Alignment & (int)Drawing::AnyBottomAlign) != 0)
			FormatFlags |= Drawing::TextFormatFlags::Bottom;
		else
			FormatFlags |= Drawing::TextFormatFlags::VerticalCenter | Drawing::TextFormatFlags::SingleLine;

		// Render to surface
		Drawing::TextRenderer::DrawText(EventArgs->Graphics, Ctrl->Text(), *Ctrl->GetFont(), LayoutRect, (State == UIX::UIXRenderState::Disabled) ? TextDisabledBrush : TextEnabledBrush, FormatFlags);
	}

	void KoreTheme::RenderControlProgressFill(const std::unique_ptr<Forms::PaintEventArgs>& EventArgs, Forms::Control* Ctrl, UIX::UIXRenderState State, uint32_t Progress) const
	{
		// Bring client rect to stack
		Drawing::Rectangle Rect(Ctrl->ClientRectangle());

		// Generate the proper gradient if enabled/disabled
		Drawing::LinearGradientBrush FillBrush(Rect, ProgressGrad1, ProgressGrad2, 90.f);

		// Render the fill to surface

		auto SmMode = EventArgs->Graphics->GetSmoothingMode();
		{
			EventArgs->Graphics->SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeAntiAlias);
			Gdiplus::RectF FillBounds(0, 0, (Rect.Width - 1.f) * (Progress / 100.0f), Rect.Height - 1.f);
			Drawing::FillRoundRectangle(EventArgs->Graphics.get(), &FillBrush, FillBounds, 2);
		}
		EventArgs->Graphics->SetSmoothingMode(SmMode);

		//EventArgs->Graphics->FillRectangle(&FillBrush, Gdiplus::RectF(2, 2, (Rect.Width - 4.f) * (Progress / 100.0f), Rect.Height - 4.f));
	}

	void KoreTheme::RenderControlGlyph(const std::unique_ptr<Forms::PaintEventArgs>& EventArgs, Forms::Control* Ctrl, UIX::UIXRenderState State) const
	{
		// Bring client rect to stack
		Drawing::Rectangle Rect(Ctrl->ClientRectangle());

		// Create Brush
		Drawing::SolidBrush FillBrush((State == UIX::UIXRenderState::Disabled) ? DisabledBorderBrush : BorderBrush);

		// It's already setup this way
		Rect.Width--;
		Rect.Height--;

		// Ensure smooth glyph
		EventArgs->Graphics->SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeAntiAlias);

		Gdiplus::GraphicsPath Path;
		Drawing::Rectangle RectCheck(Rect.Width - 18, 0, 18, Rect.Height - 1);

		// Rotate the glyph
		EventArgs->Graphics->TranslateTransform(RectCheck.X + RectCheck.Width / 2.f, RectCheck.Y + RectCheck.Height / 2.f);

		// Draw the triangle
		Path.AddLine(Drawing::PointF(-6 / 2.0f, -3 / 2.0f), Drawing::PointF(6 / 2.0f, -3 / 2.0f));
		Path.AddLine(Drawing::PointF(6 / 2.0f, -3 / 2.0f), Drawing::PointF(0, 6 / 2.0f));
		Path.CloseFigure();

		// Reset rotation
		EventArgs->Graphics->RotateTransform(0);

		// Render the glyph to surface
		EventArgs->Graphics->FillPath(&FillBrush, &Path);
	}

	void KoreTheme::RenderControlCheckBoxBorder(const std::unique_ptr<Forms::PaintEventArgs>& EventArgs, Forms::Control* Ctrl, UIX::UIXRenderState State) const
	{
		Drawing::Rectangle Rect(Ctrl->ClientRectangle());

		// Create the border brush
		auto brush = Drawing::SolidBrush((State == UIX::UIXRenderState::Disabled) ? DisabledBorderBrush : BorderBrush);
		Drawing::Pen Pen(&brush);

		// Calculate box sizing
		Drawing::Rectangle BoxRect(0, 0, Rect.Height - 1, Rect.Height - 1);
		Drawing::Rectangle FillRect(1, 1, Rect.Height - 2, Rect.Height - 2);

		// Create the fill brush
		std::unique_ptr<Drawing::Brush> FillBrush;

		// Change based on state
		switch (State)
		{
		case UIX::UIXRenderState::Default:
		case UIX::UIXRenderState::Disabled:
			FillBrush = std::make_unique<Drawing::LinearGradientBrush>(FillRect, BackgroundGrad1, BackgroundGrad2, 90.f);
			break;
		case UIX::UIXRenderState::MouseOver:
			FillBrush = std::make_unique<Drawing::LinearGradientBrush>(FillRect, BackgroundOverGrad1, BackgroundOverGrad2, 90.f);
			break;
		case UIX::UIXRenderState::Selected:
			FillBrush = std::make_unique<Drawing::LinearGradientBrush>(FillRect, ProgressGrad1, ProgressGrad2, 90.f);
			break;
		}

		// Render the boxes to surface
		EventArgs->Graphics->FillRectangle(FillBrush.get(), FillRect);
		EventArgs->Graphics->DrawRectangle(&Pen, BoxRect);
	}

	void KoreTheme::RenderControlCheckBoxCheck(const std::unique_ptr<Forms::PaintEventArgs>& EventArgs, Forms::Control* Ctrl, UIX::UIXRenderState State) const
	{
		Drawing::Rectangle Rect(Ctrl->ClientRectangle());
		Drawing::Rectangle CheckRect(((Rect.Height - 12) / 2) + 1, (Rect.Height - 12) / 2, 12, 12);

		// Render the image to the surface
		if (State == UIX::UIXRenderState::Selected)
			EventArgs->Graphics->DrawImage(CheckBoxImage, CheckRect);
	}

	void KoreTheme::RenderControlRadioBorder(const std::unique_ptr<Forms::PaintEventArgs>& EventArgs, Forms::Control* Ctrl, UIX::UIXRenderState State) const
	{
		Drawing::Rectangle Rect(Ctrl->ClientRectangle());

		// Determine which fill to use
		auto brush = Drawing::SolidBrush((State == UIX::UIXRenderState::Disabled) ? DisabledBorderBrush : BorderBrush);
		Drawing::Pen Pen(&brush);

		// Adjust for overhang
		Rect.Width--;
		Rect.Height--;

		// Render the circle to surface
		auto SmMode = EventArgs->Graphics->GetSmoothingMode();
		{
			EventArgs->Graphics->SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeAntiAlias);
			EventArgs->Graphics->DrawEllipse(&Pen, Drawing::Rectangle(0, 0, Rect.Height, Rect.Height));
		}
		EventArgs->Graphics->SetSmoothingMode(SmMode);
	}

	void KoreTheme::RenderControlRadioCheck(const std::unique_ptr<Forms::PaintEventArgs>& EventArgs, Forms::Control* Ctrl, UIX::UIXRenderState State) const
	{
		Drawing::Rectangle Rect(Ctrl->ClientRectangle());
		Drawing::Rectangle FillRect(1, 1, Rect.Height - 3, Rect.Height - 3);

		// Create the fill brush
		std::unique_ptr<Drawing::Brush> FillBrush;

		// Change based on state
		switch (State)
		{
		case UIX::UIXRenderState::Disabled:
			FillBrush = std::make_unique<Drawing::LinearGradientBrush>(FillRect, DisabledBorderBrush, DisabledBorderBrush, 90.f);
			FillRect = { 3, 3, (INT)Rect.Height - 7, (INT)Rect.Height - 7 };
			break;
		case UIX::UIXRenderState::Default:
			FillBrush = std::make_unique<Drawing::LinearGradientBrush>(FillRect, BackgroundGrad1, BackgroundGrad2, 90.f);
			break;
		case UIX::UIXRenderState::MouseOver:
			FillBrush = std::make_unique<Drawing::LinearGradientBrush>(FillRect, BackgroundOverGrad1, BackgroundOverGrad2, 90.f);
			break;
		case UIX::UIXRenderState::Selected:
			FillBrush = std::make_unique<Drawing::LinearGradientBrush>(FillRect, ProgressGrad1, ProgressGrad2, 90.f);
			FillRect = { 3, 3, (INT)Rect.Height - 7, (INT)Rect.Height - 7 };
			break;
		}

		// Render the state
		auto SmMode = EventArgs->Graphics->GetSmoothingMode();
		{
			EventArgs->Graphics->SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeAntiAlias);
			EventArgs->Graphics->FillEllipse(FillBrush.get(), FillRect);
		}
		EventArgs->Graphics->SetSmoothingMode(SmMode);
	}

	void KoreTheme::RenderControlGroupBox(const std::unique_ptr<Forms::PaintEventArgs>& EventArgs, Forms::Control* Ctrl, UIX::UIXRenderState State) const
	{
		Drawing::Rectangle Rect(Ctrl->ClientRectangle());

		Drawing::SolidBrush BackBrush(BackgroundBrush);
		Drawing::SolidBrush OutlineBrush(DarkBorderBrush);
		Drawing::SolidBrush TextBrush((State == UIX::UIXRenderState::Disabled) ? TextDisabledBrush : TextEnabledBrush);

		EventArgs->Graphics->FillRectangle(&BackBrush, Rect);

		auto Text = Ctrl->Text().ToWString();
		auto Font = Ctrl->GetFont()->GetFont();

		Gdiplus::RectF TextLayoutRect(0.0f, 0.0f, (float)Rect.Width, (float)Rect.Height);
		Gdiplus::RectF TextSize;

		EventArgs->Graphics->MeasureString((wchar_t*)Text, Text.Length(), Font.get(), TextLayoutRect, &TextSize);
		EventArgs->Graphics->DrawString((wchar_t*)Text, Text.Length(), Font.get(), Gdiplus::PointF(12, 0), &TextBrush);

		Drawing::PointF Lines[] =
		{
			// Top-left
			{0, TextSize.Height / 2.f},
			{12, TextSize.Height / 2.f},
			// Left
			{0, TextSize.Height / 2.f},
			{0, Rect.Height - 1.f},
			// Bottom
			{0, Rect.Height - 1.f},
			{Rect.Width - 1.f, Rect.Height - 1.f},
			// Right
			{Rect.Width - 1.f, Rect.Height - 1.f},
			{Rect.Width - 1.f, TextSize.Height / 2.f},
			// Top-right
			{TextSize.Width + 11.f, TextSize.Height / 2.f},
			{Rect.Width - 1.f, TextSize.Height / 2.f}
		};

		auto pen = Drawing::Pen(&OutlineBrush);
		EventArgs->Graphics->DrawLines(&pen, &Lines[0], _countof(Lines));
	}

	void KoreTheme::RenderControlListColumnHeader(const std::unique_ptr<Forms::DrawListViewColumnHeaderEventArgs>& EventArgs, Forms::Control* Ctrl) const
	{
		// Fetch color from foreground
		Drawing::SolidBrush BackBrush(HeaderBrush);

		// Render to the surface
		Drawing::Rectangle NewBounds(EventArgs->Bounds);

		NewBounds.Inflate(0, -1);
		NewBounds.Width -= 2;

		auto SmMode = EventArgs->Graphics->GetSmoothingMode();
		{
			EventArgs->Graphics->SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeAntiAlias);
			Drawing::FillRoundRectangle(EventArgs->Graphics.get(), &BackBrush, NewBounds, 2);
		}
		EventArgs->Graphics->SetSmoothingMode(SmMode);

		// Shift control bounds for rendering text
		NewBounds.X += 5;
		NewBounds.Width -= 5;

		Drawing::TextRenderer::DrawText(EventArgs->Graphics, EventArgs->Header->Text(), *Ctrl->GetFont(), NewBounds,  Drawing::Color::White, Drawing::TextFormatFlags::Left | Drawing::TextFormatFlags::VerticalCenter | Drawing::TextFormatFlags::SingleLine | Drawing::TextFormatFlags::EndEllipsis);
	}

	void KoreTheme::RenderControlListHeader(const std::unique_ptr<Drawing::BufferedGraphics>& EventArgs) const
	{
		auto brush = Drawing::SolidBrush(BackgroundLightBrush);
		EventArgs->Graphics->FillRectangle(&brush, EventArgs->Region());
		Drawing::SolidBrush BackBrush(HeaderBrush);
		Drawing::Rectangle NewBounds(EventArgs->Region());

		NewBounds.Inflate(0, -1);
		NewBounds.Width -= 2;

		auto SmMode = EventArgs->Graphics->GetSmoothingMode();
		{
			EventArgs->Graphics->SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeAntiAlias);
			Drawing::FillRoundRectangle(EventArgs->Graphics.get(), &BackBrush, NewBounds, 2);
		}
		EventArgs->Graphics->SetSmoothingMode(SmMode);
		EventArgs->Render();
	}

	void KoreTheme::RenderControlListItem(const std::unique_ptr<Forms::DrawListViewItemEventArgs>& EventArgs, Forms::Control* Ctrl, Drawing::Rectangle SubItemBounds) const
	{
		// This isn't used for modern rendering.
	}

	void KoreTheme::RenderControlListSubItem(const std::unique_ptr<Forms::DrawListViewSubItemEventArgs>& EventArgs, Forms::Control* Ctrl) const
	{
		// Use stock bounds, subitems are valid
		Drawing::Rectangle SubItemBounds(EventArgs->Bounds);

		// Fetch the state because we are owner draw
		auto State = SendMessageA(Ctrl->GetHandle(), LVM_GETITEMSTATE, (WPARAM)EventArgs->ItemIndex, (LPARAM)LVIS_SELECTED);

		// Fetch color from style
		Drawing::SolidBrush BackBrush((State == LVIS_SELECTED) ? BorderBrush : EventArgs->Style.BackColor);

		// Build text layout rect
		Drawing::Rectangle TextLayoutRect(SubItemBounds);

		// Pad start and end
		TextLayoutRect.X += 1;
		TextLayoutRect.Width -= 2;

		// Render background first, then text
		EventArgs->Graphics->FillRectangle(&BackBrush, SubItemBounds);
		Drawing::TextRenderer::DrawText(EventArgs->Graphics, EventArgs->Text, *Ctrl->GetFont(), TextLayoutRect, (State == LVIS_SELECTED) ? Drawing::Color::White : EventArgs->Style.ForeColor, Drawing::TextFormatFlags::Left | Drawing::TextFormatFlags::VerticalCenter | Drawing::TextFormatFlags::SingleLine | Drawing::TextFormatFlags::EndEllipsis);
	}

	void KoreTheme::RenderControlToolTip(const std::unique_ptr<Forms::DrawToolTipEventArgs>& EventArgs, Forms::Control* Ctrl) const
	{
		Drawing::SolidBrush Brush(BackgroundLightBrush);

		// Render to surface
		EventArgs->Graphics->FillRectangle(&Brush, EventArgs->Bounds);

		Drawing::Rectangle BoundsOne(EventArgs->Bounds);
		BoundsOne.Inflate(-1, -1);


		auto brush = Drawing::SolidBrush(BorderBrush);
		auto pen = Drawing::Pen(&brush);

		EventArgs->Graphics->DrawRectangle(&pen, BoundsOne);

		Drawing::SolidBrush TextBrush(TextEnabledBrush);
		Gdiplus::RectF BoundsF((float)EventArgs->Bounds.X, (float)EventArgs->Bounds.Y, (float)EventArgs->Bounds.Width, (float)EventArgs->Bounds.Height);
		
		Gdiplus::StringFormat Fmt;
		Fmt.SetAlignment(Gdiplus::StringAlignment::StringAlignmentCenter);
		Fmt.SetLineAlignment(Gdiplus::StringAlignment::StringAlignmentCenter);

		auto WideString = Ctrl->Text().ToWString();
		auto Fnt = Ctrl->GetFont()->GetFont();

		// Render text
		EventArgs->Graphics->DrawString((wchar_t*)WideString, WideString.Length(), Fnt.get(), BoundsF, &Fmt, &TextBrush);
	}

	Drawing::Color KoreTheme::GetRenderColor(UIX::UIXRenderColor Color) const
	{
		switch (Color)
		{
		case UIX::UIXRenderColor::TextDefault:
			return TextEnabledBrush;
		case UIX::UIXRenderColor::BackgroundDefault:
			return BackgroundBrush;
		case UIX::UIXRenderColor::BackgroundLight:
			return BackgroundLightBrush;
		default:
			return Drawing::Color();
		}
	}
}
