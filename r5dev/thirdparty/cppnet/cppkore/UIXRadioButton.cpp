#include "stdafx.h"
#include "UIXRadioButton.h"
#include "UIXTheme.h"

namespace UIX
{
	UIXRadioButton::UIXRadioButton()
		: RadioButton()
	{
		SetStyle(ControlStyles::AllPaintingInWmPaint | ControlStyles::DoubleBuffer, true);

		this->SetOwnerDraw(true);
	}

	void UIXRadioButton::OnPaint(const std::unique_ptr<PaintEventArgs>& EventArgs)
	{
		auto State = UIX::UIXRenderState::Default;
		auto Renderer = UIXTheme::GetRenderer();

		if (!this->Enabled())
			State = UIX::UIXRenderState::Disabled;
		else if (this->Checked())
			State = UIX::UIXRenderState::Selected;
		else if (GetFlag(Forms::ButtonFlags::FlagMouseOver))
			State = UIX::UIXRenderState::MouseOver;

		Renderer->RenderControlBackground(EventArgs, this, State);
		Renderer->RenderControlRadioBorder(EventArgs, this, State);
		Renderer->RenderControlRadioCheck(EventArgs, this, State);

		// Padding for the border
		Drawing::Rectangle TextLayoutRect(this->ClientRectangle());
		TextLayoutRect.X = (this->_ClientHeight - 1) + 4;

		Renderer->RenderControlText(EventArgs, this, State, TextLayoutRect, Drawing::ContentAlignment::MiddleLeft);
	}
}
