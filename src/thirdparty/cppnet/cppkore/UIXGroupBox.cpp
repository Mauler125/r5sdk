#include "stdafx.h"
#include "UIXGroupBox.h"
#include "UIXTheme.h"

namespace UIX
{
	UIXGroupBox::UIXGroupBox()
		: GroupBox()
	{
		this->SetStyle(ControlStyles::AllPaintingInWmPaint | ControlStyles::UserPaint | ControlStyles::DoubleBuffer, true);
	}

	void UIXGroupBox::OnPaint(const std::unique_ptr<PaintEventArgs>& EventArgs)
	{
		auto State = UIX::UIXRenderState::Default;
		auto Renderer = UIXTheme::GetRenderer();

		if (!this->Enabled())
			State = UIX::UIXRenderState::Disabled;

		Renderer->RenderControlGroupBox(EventArgs, this, State);
	}
}
