#include "stdafx.h"
#include "UIXTextBox.h"
#include "UIXTheme.h"

namespace UIX
{
	UIXTextBox::UIXTextBox()
		: TextBox()
	{
		this->SetBorderStyle(BorderStyle::None);

		auto Renderer = UIXTheme::GetRenderer();

		this->SetBackColor(Renderer->GetRenderColor(UIX::UIXRenderColor::BackgroundDefault));
		this->SetForeColor(Renderer->GetRenderColor(UIX::UIXRenderColor::TextDefault));
	}

	void UIXTextBox::WndProc(Message& Msg)
	{
		switch (Msg.Msg)
		{
		case WM_NCPAINT:
			WmNcPaint(Msg);
			break;
		default:
			TextBox::WndProc(Msg);
			break;
		}
	}

	void UIXTextBox::WmNcPaint(Message& Msg)
	{
		// Handle base logic first
		TextBox::WndProc(Msg);

		// Generate paint event args here...
		HDC hDC = GetWindowDC((HWND)Msg.HWnd);
		auto EventArgs = std::make_unique<PaintEventArgs>(hDC, this->ClientRectangle());
		auto Renderer = UIXTheme::GetRenderer();

		// Render the border
		Renderer->RenderControlBorder(EventArgs, this, UIX::UIXRenderState::Default);

		// Must release the handle here
		ReleaseDC((HWND)Msg.HWnd, hDC);
	}
}
