#pragma once

#include "Label.h"

using namespace Forms;

namespace UIX
{
	// Represents a UIX themed label control.
	class UIXLabel : public Label
	{
	public:
		UIXLabel();

		// Override the paint event to provide our custom label control.
		virtual void OnPaint(const std::unique_ptr<PaintEventArgs>& EventArgs);
	};
}