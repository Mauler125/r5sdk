#pragma once

#include <cstdint>
#include "Control.h"
#include "FlatStyle.h"
#include "BorderStyle.h"
#include "ContentAlignment.h"

namespace Forms
{
	// Encapsulates a standard Windows group box.
	class GroupBox : public Control
	{
	public:
		GroupBox();
		virtual ~GroupBox() = default;

		// Gets the drawing mode of the group box control.
		bool OwnerDraw();
		// Sets the drawing mode of the group box control.
		void SetOwnerDraw(bool Value);

		// Gets the flat style appearance of the label control.
		FlatStyle GetFlatStyle();
		// Sets the flat style appearance of the label control.
		void SetFlatStyle(FlatStyle Value);

		// Add a control to this group box.
		virtual void AddControl(Control* Ctrl);

		// Override WndProc for specific group box messages.
		virtual void WndProc(Message& Msg);

	protected:
		// Get custom control creation parameters for this instance.
		virtual CreateParams GetCreateParams();

	private:
		// Whether or not the control will draw itself
		bool _OwnerDraw;
		// Controls the style of the control
		FlatStyle _FlatStyle;

		// We must define each window message handler here...
		void WmEraseBkgnd(Message& Msg);
	};
}