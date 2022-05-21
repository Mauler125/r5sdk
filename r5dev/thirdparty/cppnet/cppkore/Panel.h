#pragma once

#include <cstdint>
#include "Control.h"
#include "BorderStyle.h"

namespace Forms
{
	// Represents a panel control.
	class Panel : public Control
	{
	public:
		Panel();
		virtual ~Panel() = default;

		// Gets the border style appearence of the panel control.
		BorderStyle GetBorderStyle();
		// Sets the border style appearence of the panel control.
		void SetBorderStyle(BorderStyle Value);

		// Add a control to this panel.
		virtual void AddControl(Control* Ctrl);

	protected:
		// Get custom control creation parameters for this instance.
		virtual CreateParams GetCreateParams();

	private:
		// Internal cached flags
		BorderStyle _BorderStyle;
	};
}