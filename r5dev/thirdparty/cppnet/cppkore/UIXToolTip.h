#pragma once

#include "ToolTip.h"

using namespace Forms;

namespace UIX
{
	// Represents a UIX themed tooltip control.
	class UIXToolTip : public ToolTip
	{
	public:
		UIXToolTip();

		// Override the draw event to provide our custom tooltip control.
		virtual void OnDraw(const std::unique_ptr<DrawToolTipEventArgs>& EventArgs);
	};
}