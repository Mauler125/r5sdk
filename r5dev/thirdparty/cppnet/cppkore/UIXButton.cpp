#include "stdafx.h"
#include "UIXButton.h"
#include "UIXTheme.h"

namespace UIX
{
	UIXButton::UIXButton()
		: Button()
	{
		SetStyle(ControlStyles::AllPaintingInWmPaint | ControlStyles::DoubleBuffer, true);

		this->SetOwnerDraw(true);
	}

	void UIXButton::OnPaint(const std::unique_ptr<PaintEventArgs>& EventArgs)
	{
		auto State = UIX::UIXRenderState::Default;
		auto Renderer = UIXTheme::GetRenderer();

		if (!this->Enabled())
			State = UIX::UIXRenderState::Disabled;
		else if (GetFlag(ButtonFlags::FlagMouseDown))
			State = UIX::UIXRenderState::MouseDown;
		else if (GetFlag(ButtonFlags::FlagMouseOver))
			State = UIX::UIXRenderState::MouseOver;

		Renderer->RenderControlButtonBackground(EventArgs, this, State);
		Renderer->RenderControlBorder(EventArgs, this, State);
		Renderer->RenderControlText(EventArgs, this, State, this->ClientRectangle(), Drawing::ContentAlignment::MiddleCenter);
	}
}
