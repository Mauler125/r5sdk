#pragma once

#include "GroupBox.h"

using namespace Forms;

namespace UIX
{
	// Represents a UIX themed group box control.
	class UIXGroupBox : public GroupBox
	{
	public:
		UIXGroupBox();

		// Override the paint event to provide our custom group box control.
		virtual void OnPaint(const std::unique_ptr<PaintEventArgs>& EventArgs);
	};
}