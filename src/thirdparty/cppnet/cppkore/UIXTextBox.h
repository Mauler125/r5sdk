#pragma once

#include "TextBox.h"

using namespace Forms;

namespace UIX
{
	// Represents a UIX themed text box control.
	class UIXTextBox : public TextBox
	{
	public:
		UIXTextBox();

		// The standard windows message pump for this control.
		virtual void WndProc(Message& Msg);

	private:

		// We must define each window message handler here...
		void WmNcPaint(Message& Msg);
	};
}