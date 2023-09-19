#pragma once

#include "CheckBox.h"

using namespace Forms;

namespace UIX
{
	// Represents a UIX themed check box control.
	class UIXCheckBox : public CheckBox
	{
	public:
		UIXCheckBox();

		// Override the paint even to provide our custom check box control.
		virtual void OnPaint(const std::unique_ptr<PaintEventArgs>& EventArgs);
	};
}