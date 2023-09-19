#include "stdafx.h"
#include "UIXProgressBar.h"
#include "UIXTheme.h"
#include "TextRenderer.h"

namespace UIX
{
	UIXProgressBar::UIXProgressBar()
		: ProgressBar()
	{
		SetStyle(ControlStyles::AllPaintingInWmPaint | ControlStyles::UserPaint | ControlStyles::DoubleBuffer, true);
	}

	void UIXProgressBar::OnPaint(const std::unique_ptr<PaintEventArgs>& EventArgs)
	{
		auto State = UIX::UIXRenderState::Default;
		auto Renderer = UIXTheme::GetRenderer();

		if (!this->Enabled())
			State = UIX::UIXRenderState::Disabled;

		Renderer->RenderControlBackground(EventArgs, this, State);
		Renderer->RenderControlBorder(EventArgs, this, State);
		Renderer->RenderControlProgressFill(EventArgs, this, State, this->Value());
	}
}
