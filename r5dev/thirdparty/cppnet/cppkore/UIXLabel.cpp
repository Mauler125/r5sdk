#include "stdafx.h"
#include "UIXLabel.h"
#include "UIXTheme.h"

namespace UIX
{
	UIXLabel::UIXLabel()
		: Label()
	{
		this->SetOwnerDraw(true);
	}

	void UIXLabel::OnPaint(const std::unique_ptr<PaintEventArgs>& EventArgs)
	{
		auto State = UIX::UIXRenderState::Default;
		auto Renderer = UIXTheme::GetRenderer();

		if (!this->Enabled())
			State = UIX::UIXRenderState::Disabled;

		Renderer->RenderControlBackground(EventArgs, this, State);
		Renderer->RenderControlText(EventArgs, this, State, this->ClientRectangle(), this->TextAlign());
	}
}
