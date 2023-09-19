#include "stdafx.h"
#include "UIXComboBox.h"
#include "UIXTheme.h"

namespace UIX
{
	UIXComboBox::UIXComboBox()
		: ComboBox(), _MouseOver(false)
	{
		this->SetStyle(ControlStyles::AllPaintingInWmPaint | ControlStyles::DoubleBuffer | ControlStyles::UserPaint, true);

		auto Renderer = UIXTheme::GetRenderer();

		this->SetBackColor(Renderer->GetRenderColor(UIX::UIXRenderColor::BackgroundDefault));
		this->SetForeColor(Renderer->GetRenderColor(UIX::UIXRenderColor::TextDefault));
	}

	void UIXComboBox::OnPaint(const std::unique_ptr<PaintEventArgs>& EventArgs)
	{
		auto State = UIX::UIXRenderState::Default;
		auto Renderer = UIXTheme::GetRenderer();

		if (!this->Enabled())
			State = UIX::UIXRenderState::Disabled;
		else if (_MouseOver)
			State = UIX::UIXRenderState::MouseOver;

		// Padding for the border
		Drawing::Rectangle TextRect(this->ClientRectangle());
		TextRect.X = 3;

		Renderer->RenderControlButtonBackground(EventArgs, this, State);
		Renderer->RenderControlBorder(EventArgs, this, State);
		Renderer->RenderControlText(EventArgs, this, State, TextRect, Drawing::ContentAlignment::MiddleLeft);
		Renderer->RenderControlGlyph(EventArgs, this, State);
	}

	void UIXComboBox::OnMouseEnter()
	{
		ComboBox::OnMouseEnter();
		_MouseOver = true;
	}

	void UIXComboBox::OnMouseLeave()
	{
		ComboBox::OnMouseLeave();
		_MouseOver = false;
	}
}
