#include "stdafx.h"
#include "UIXCheckBox.h"
#include "UIXTheme.h"

namespace UIX
{
	UIXCheckBox::UIXCheckBox()
		: CheckBox()
	{
		SetStyle(ControlStyles::AllPaintingInWmPaint | ControlStyles::DoubleBuffer, true);

		this->SetOwnerDraw(true);
	}

	void UIXCheckBox::OnPaint(const std::unique_ptr<PaintEventArgs>& EventArgs)
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
		Renderer->RenderControlCheckBoxBorder(EventArgs, this, State);
		Renderer->RenderControlCheckBoxCheck(EventArgs, this, State);

		// Padding for the border
		Drawing::Rectangle TextLayoutRect(this->ClientRectangle());
		TextLayoutRect.X = (this->_ClientHeight - 1) + 4;

		Renderer->RenderControlText(EventArgs, this, State, TextLayoutRect, Drawing::ContentAlignment::MiddleLeft);
	}
}
