#pragma once

#include "Button.h"

using namespace Forms;

namespace UIX
{
	// Represents a UIX themed button control.
	class UIXButton : public Button
	{
	public:
		UIXButton();

		// Override the paint event to provide our custom button control.
		virtual void OnPaint(const std::unique_ptr<PaintEventArgs>& EventArgs);
	};
}