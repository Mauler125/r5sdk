#include "stdafx.h"
#include "WraithTheme.h"
#include "MessageBox.h"
#include "CheckBoxImage.h"

namespace Themes
{
	// Constants for brushes
	const static auto BorderBrush = Drawing::Color(3, 169, 244);
	const static auto DisabledBorderBrush = Drawing::Color(160, 160, 160);

	const static auto BackgroundBrush = Drawing::Color(33, 33, 33);

	const static auto BackgroundGrad1 = Drawing::Color(40, 40, 40);
	const static auto BackgroundGrad2 = Drawing::Color(30, 30, 30);

	const static auto BackgroundOverGrad1 = Drawing::Color(50, 50, 50);
	const static auto BackgroundOverGrad2 = Drawing::Color(40, 40, 40);

	const static auto TextEnabledBrush = Drawing::Color(Drawing::Color::White);
	const static auto TextDisabledBrush = Drawing::Color(Drawing::Color::Red);

	const static auto ProgressGrad1 = Drawing::Color(3, 169, 244);
	const static auto ProgressGrad2 = Drawing::Color(0, 130, 220);

	const static auto HeaderGrad1 = Drawing::Color(50, 50, 50);
	const static auto HeaderGrad2 = Drawing::Color(42, 42, 42);

	// Constants for images
	static Drawing::Image* CheckBoxImage = nullptr;

	WraithTheme::WraithTheme()
		: UIX::UIXRenderer()
	{
		CheckBoxImage = Drawing::ImageFromTgaData(CheckBoxImage_Src, sizeof(CheckBoxImage_Src));

		// Change the message box colors to represent our theme
		Forms::MessageBox::SetMessageBoxColors(Drawing::Color::White, Drawing::Color(30, 30, 30), Drawing::Color(50, 50, 50));
	}

	WraithTheme::~WraithTheme()
	{
		delete CheckBoxImage;
	}

	void WraithTheme::RenderControlBorder(const std::unique_ptr<Forms::PaintEventArgs>& EventArgs, Forms::Control* Ctrl, UIX::UIXRenderState State) const
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

		auto Brush = std::make_unique<Drawing::SolidBrush>((State == UIX::UIXRenderState::Disabled) ? DisabledBorderBrush : BorderBrush);

		Drawing::Pen Pen(Brush.get());

		EventArgs->Graphics->DrawRectangle(&Pen, Rect);
	}

	void WraithTheme::RenderControlBackground(const std::unique_ptr<Forms::PaintEventArgs>& EventArgs, Forms::Control* Ctrl, UIX::UIXRenderState State) const
	{
		Drawing::Rectangle Rect(Ctrl->ClientRectangle());

		auto brush = Drawing::SolidBrush(BackgroundBrush);

		EventArgs->Graphics->FillRectangle(&brush, Rect);
	}

	void WraithTheme::RenderControlButtonBackground(const std::unique_ptr<Forms::PaintEventArgs>& EventArgs, Forms::Control* Ctrl, UIX::UIXRenderState State) const
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

	void WraithTheme::RenderControlText(const std::unique_ptr<Forms::PaintEventArgs>& EventArgs, Forms::Control* Ctrl, UIX::UIXRenderState State, Drawing::Rectangle LayoutRect, Drawing::ContentAlignment Alignment) const
	{
		// Fetch color from foreground
		Drawing::SolidBrush TextBrush((State == UIX::UIXRenderState::Disabled) ? TextDisabledBrush : TextEnabledBrush);

		// Fetch control text and font handle
		auto Text = Ctrl->Text().ToWString();
		auto Font = Ctrl->GetFont()->GetFont();

		// Setup string formatting
		Gdiplus::StringFormat StrFmt;

		// Handle horizontal alignment
		if (((int)Alignment & (int)Drawing::AnyLeftAlign) != 0)
			StrFmt.SetAlignment(Gdiplus::StringAlignment::StringAlignmentNear);
		else if (((int)Alignment & (int)Drawing::AnyRightAlign) != 0)
			StrFmt.SetAlignment(Gdiplus::StringAlignment::StringAlignmentFar);
		else
			StrFmt.SetAlignment(Gdiplus::StringAlignment::StringAlignmentCenter);

		// Handle vertical alignment
		if (((int)Alignment & (int)Drawing::AnyTopAlign) != 0)
			StrFmt.SetLineAlignment(Gdiplus::StringAlignment::StringAlignmentNear);
		else if (((int)Alignment & (int)Drawing::AnyBottomAlign) != 0)
			StrFmt.SetLineAlignment(Gdiplus::StringAlignment::StringAlignmentFar);
		else
			StrFmt.SetLineAlignment(Gdiplus::StringAlignment::StringAlignmentCenter);

		// Build text layout rect
		Gdiplus::RectF TextLayoutRect((float)LayoutRect.X, (float)LayoutRect.Y, (float)LayoutRect.Width, (float)LayoutRect.Height);

		// Render text to the surface
		EventArgs->Graphics->DrawString((wchar_t*)Text, Text.Length(), Font.get(), TextLayoutRect, &StrFmt, &TextBrush);
	}

	void WraithTheme::RenderControlProgressFill(const std::unique_ptr<Forms::PaintEventArgs>& EventArgs, Forms::Control* Ctrl, UIX::UIXRenderState State, uint32_t Progress) const
	{
		// Bring client rect to stack
		Drawing::Rectangle Rect(Ctrl->ClientRectangle());

		// Generate the proper gradient if enabled/disabled
		Drawing::LinearGradientBrush FillBrush(Rect, ProgressGrad1, ProgressGrad2, 90.f);

		// Render the fill to surface
		EventArgs->Graphics->FillRectangle(&FillBrush, Gdiplus::RectF(2, 2, (Rect.Width - 4.f) * (Progress / 100.0f), Rect.Height - 4.f));
	}

	void WraithTheme::RenderControlGlyph(const std::unique_ptr<Forms::PaintEventArgs>& EventArgs, Forms::Control* Ctrl, UIX::UIXRenderState State) const
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

	void WraithTheme::RenderControlCheckBoxBorder(const std::unique_ptr<Forms::PaintEventArgs>& EventArgs, Forms::Control* Ctrl, UIX::UIXRenderState State) const
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
		case UIX::UIXRenderState::Disabled:
			FillBrush = std::make_unique<Drawing::LinearGradientBrush>(FillRect, DisabledBorderBrush, DisabledBorderBrush, 90.f);
			break;
		case UIX::UIXRenderState::Default:
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

	void WraithTheme::RenderControlCheckBoxCheck(const std::unique_ptr<Forms::PaintEventArgs>& EventArgs, Forms::Control* Ctrl, UIX::UIXRenderState State) const
	{
		Drawing::Rectangle Rect(Ctrl->ClientRectangle());
		Drawing::Rectangle CheckRect(((Rect.Height - 12) / 2) + 1, (Rect.Height - 12) / 2, 12, 12);

		// Render the image to the surface
		if (State == UIX::UIXRenderState::Selected)
			EventArgs->Graphics->DrawImage(CheckBoxImage, CheckRect);
	}

	void WraithTheme::RenderControlRadioBorder(const std::unique_ptr<Forms::PaintEventArgs>& EventArgs, Forms::Control* Ctrl, UIX::UIXRenderState State) const
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

	void WraithTheme::RenderControlRadioCheck(const std::unique_ptr<Forms::PaintEventArgs>& EventArgs, Forms::Control* Ctrl, UIX::UIXRenderState State) const
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

	void WraithTheme::RenderControlGroupBox(const std::unique_ptr<Forms::PaintEventArgs>& EventArgs, Forms::Control* Ctrl, UIX::UIXRenderState State) const
	{
		Drawing::Rectangle Rect(Ctrl->ClientRectangle());

		Drawing::SolidBrush BackBrush(BackgroundBrush);
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

		auto pen = Drawing::Pen(&TextBrush);
		EventArgs->Graphics->DrawLines(&pen, &Lines[0], _countof(Lines));
	}

	void WraithTheme::RenderControlListColumnHeader(const std::unique_ptr<Forms::DrawListViewColumnHeaderEventArgs>& EventArgs, Forms::Control* Ctrl) const
	{
		// Fetch color from foreground
		Drawing::SolidBrush TextBrush(TextEnabledBrush);
		Drawing::LinearGradientBrush BackBrush(EventArgs->Bounds, HeaderGrad1, HeaderGrad2, 90.f);

		// Fetch control text and font handle
		auto Text = EventArgs->Header->Text().ToWString();
		auto Font = Ctrl->GetFont()->GetFont();

		// Setup string formatting
		Gdiplus::StringFormat StrFmt;
		StrFmt.SetAlignment(Gdiplus::StringAlignment::StringAlignmentNear);
		StrFmt.SetLineAlignment(Gdiplus::StringAlignment::StringAlignmentCenter);
		StrFmt.SetTrimming(Gdiplus::StringTrimming::StringTrimmingEllipsisCharacter);

		// Build text layout rect
		Gdiplus::RectF TextLayoutRect((float)EventArgs->Bounds.X + 3, (float)EventArgs->Bounds.Y, (float)EventArgs->Bounds.Width - 6, (float)EventArgs->Bounds.Height);
		Gdiplus::RectF TextSize;

		EventArgs->Graphics->MeasureString((wchar_t*)Text, Text.Length(), Font.get(), Drawing::PointF(0, 0), &TextSize);

		TextLayoutRect.Height = TextSize.Height - 1;
		TextLayoutRect.Y = (TextLayoutRect.Y) + (TextSize.Height / 2.f);
		TextLayoutRect.Y--;	// There is always one overlap pixel at bottom/right

		// Render to the surface
		//EventArgs->Graphics->FillRectangle(&BackBrush, EventArgs->Bounds);
		Drawing::Rectangle NewBounds(EventArgs->Bounds);

		NewBounds.Inflate(0, -1);
		NewBounds.Width -= 2;

		auto SmMode = EventArgs->Graphics->GetSmoothingMode();
		{
			EventArgs->Graphics->SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeAntiAlias);
			Drawing::FillRoundRectangle(EventArgs->Graphics.get(), &BackBrush, NewBounds, 4);
		}
		EventArgs->Graphics->SetSmoothingMode(SmMode);

		EventArgs->Graphics->DrawString((wchar_t*)Text, Text.Length(), Font.get(), TextLayoutRect, &StrFmt, &TextBrush);
		//EventArgs->Graphics->DrawLine(&Drawing::Pen(&Drawing::SolidBrush(Drawing::Color::Black)), Drawing::Point(EventArgs->Bounds.X, EventArgs->Bounds.Height - 1), Drawing::Point(EventArgs->Bounds.X + EventArgs->Bounds.Width, EventArgs->Bounds.Height - 1));
	}

	void WraithTheme::RenderControlListHeader(const std::unique_ptr<Drawing::BufferedGraphics>& EventArgs) const
	{
		auto brush = Drawing::SolidBrush(BackgroundBrush);
		EventArgs->Graphics->FillRectangle(&brush, EventArgs->Region());
		Drawing::LinearGradientBrush BackBrush(EventArgs->Region(), HeaderGrad1, HeaderGrad2, 90.f);
		Drawing::Rectangle NewBounds(EventArgs->Region());

		NewBounds.Inflate(0, -1);
		NewBounds.Width -= 2;

		auto SmMode = EventArgs->Graphics->GetSmoothingMode();
		{
			EventArgs->Graphics->SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeAntiAlias);
			Drawing::FillRoundRectangle(EventArgs->Graphics.get(), &BackBrush, NewBounds, 4);
		}
		EventArgs->Graphics->SetSmoothingMode(SmMode);
	}

	void WraithTheme::RenderControlListItem(const std::unique_ptr<Forms::DrawListViewItemEventArgs>& EventArgs, Forms::Control* Ctrl, Drawing::Rectangle SubItemBounds) const
	{
		// Fetch the state because we are owner draw
		auto State = SendMessageA(Ctrl->GetHandle(), LVM_GETITEMSTATE, (WPARAM)EventArgs->ItemIndex, (LPARAM)LVIS_SELECTED);

		// Fetch color from style
		Drawing::SolidBrush TextBrush((State == LVIS_SELECTED) ? Drawing::Color::White : EventArgs->Style.ForeColor);
		Drawing::SolidBrush BackBrush((State == LVIS_SELECTED) ? BorderBrush :  EventArgs->Style.BackColor);

		// Fetch control text and font handle
		auto Text = EventArgs->Text.ToWString();
		auto Font = Ctrl->GetFont()->GetFont();

		// Setup string formatting
		Gdiplus::StringFormat StrFmt;
		StrFmt.SetAlignment(Gdiplus::StringAlignment::StringAlignmentNear);
		StrFmt.SetLineAlignment(Gdiplus::StringAlignment::StringAlignmentCenter);
		StrFmt.SetTrimming(Gdiplus::StringTrimming::StringTrimmingEllipsisCharacter);
		StrFmt.SetFormatFlags(Gdiplus::StringFormatFlags::StringFormatFlagsNoWrap);

		// Build text layout rect
		Gdiplus::RectF TextLayoutRect((float)SubItemBounds.X, (float)SubItemBounds.Y, (float)SubItemBounds.Width, (float)SubItemBounds.Height);
		Gdiplus::RectF TextSize;

		EventArgs->Graphics->MeasureString((wchar_t*)Text, Text.Length(), Font.get(), Drawing::PointF(0, 0), &TextSize);

		TextLayoutRect.Height = TextSize.Height;
		TextLayoutRect.Y = (TextLayoutRect.Y) + ((SubItemBounds.Height / 2.f) - (TextSize.Height / 2.f));

		EventArgs->Graphics->FillRectangle(&BackBrush, SubItemBounds);
		EventArgs->Graphics->DrawString((wchar_t*)Text, Text.Length(), Font.get(), TextLayoutRect, &StrFmt, &TextBrush);		
	}

	void WraithTheme::RenderControlListSubItem(const std::unique_ptr<Forms::DrawListViewSubItemEventArgs>& EventArgs, Forms::Control* Ctrl) const
	{
		// Use stock bounds, subitems are valid
		Drawing::Rectangle SubItemBounds(EventArgs->Bounds);

		// Fetch the state because we are owner draw
		auto State = SendMessageA(Ctrl->GetHandle(), LVM_GETITEMSTATE, (WPARAM)EventArgs->ItemIndex, (LPARAM)LVIS_SELECTED);

		// Fetch color from style
		Drawing::SolidBrush TextBrush((State == LVIS_SELECTED) ? Drawing::Color::White : EventArgs->Style.ForeColor);
		Drawing::SolidBrush BackBrush((State == LVIS_SELECTED) ? BorderBrush : EventArgs->Style.BackColor);

		// Fetch control text and font handle
		auto Text = EventArgs->Text.ToWString();
		auto Font = Ctrl->GetFont()->GetFont();

		// Setup string formatting
		Gdiplus::StringFormat StrFmt;
		StrFmt.SetAlignment(Gdiplus::StringAlignment::StringAlignmentNear);
		StrFmt.SetLineAlignment(Gdiplus::StringAlignment::StringAlignmentCenter);
		StrFmt.SetTrimming(Gdiplus::StringTrimming::StringTrimmingEllipsisCharacter);
		StrFmt.SetFormatFlags(Gdiplus::StringFormatFlags::StringFormatFlagsNoWrap);

		// Build text layout rect
		Gdiplus::RectF TextLayoutRect((float)SubItemBounds.X, (float)SubItemBounds.Y, (float)SubItemBounds.Width, (float)SubItemBounds.Height);
		Gdiplus::RectF TextSize;

		EventArgs->Graphics->MeasureString((wchar_t*)Text, Text.Length(), Font.get(), Drawing::PointF(0, 0), &TextSize);

		TextLayoutRect.Height = TextSize.Height;
		TextLayoutRect.Y = (TextLayoutRect.Y) + ((SubItemBounds.Height / 2.f) - (TextSize.Height / 2.f));

		EventArgs->Graphics->FillRectangle(&BackBrush, SubItemBounds);
		EventArgs->Graphics->DrawString((wchar_t*)Text, Text.Length(), Font.get(), TextLayoutRect, &StrFmt, &TextBrush);
	}

	Drawing::Color WraithTheme::GetRenderColor(UIX::UIXRenderColor Color) const
	{
		switch (Color)
		{
		case UIX::UIXRenderColor::TextDefault:
			return TextEnabledBrush;
		case UIX::UIXRenderColor::BackgroundDefault:
			return BackgroundBrush;
		default:
			return Drawing::Color();
		}
	}
}
