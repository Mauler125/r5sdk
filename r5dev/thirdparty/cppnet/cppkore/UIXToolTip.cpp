#include "stdafx.h"
#include "UIXToolTip.h"
#include "UIXTheme.h"

namespace UIX
{
	UIXToolTip::UIXToolTip()
		: ToolTip()
	{
		this->SetOwnerDraw(true);
	}

	void UIXToolTip::OnDraw(const std::unique_ptr<DrawToolTipEventArgs>& EventArgs)
	{
		// Call the base event for processing
		ToolTip::OnDraw(EventArgs);

		// Handle our own drawing
		UIXTheme::GetRenderer()->RenderControlToolTip(EventArgs, this);
	}
}
