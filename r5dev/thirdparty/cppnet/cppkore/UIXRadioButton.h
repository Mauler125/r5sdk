#pragma once

#include "RadioButton.h"

using namespace Forms;

namespace UIX
{
	// Represents a UIX themed radio button control.
	class UIXRadioButton : public RadioButton
	{
	public:
		UIXRadioButton();

		// Override the paint event to provide our custom radio button control.
		virtual void OnPaint(const std::unique_ptr<PaintEventArgs>& EventArgs);
	};
}