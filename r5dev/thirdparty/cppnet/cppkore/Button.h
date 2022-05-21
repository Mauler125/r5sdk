#pragma once

#include <cstdint>
#include "Control.h"
#include "ButtonBase.h"
#include "DialogResult.h"

namespace Forms
{
	// Represents a Windows button.
	class Button : public ButtonBase
	{
	public:
		Button();
		virtual ~Button() = default;

		// Gets a value that is returned to the
		// parent form when the button is clicked.
		DialogResult GetDialogResult();
		// Sets a value that is returned to the
		// parent form when the button is clicked.
		void SetDialogResult(DialogResult Value);

		// Generates a click event for a button.
		void PerformClick();

		// Changes the default action status.
		void NotifyDefault(bool Value);

		// We must define base events here
		virtual void OnClick();
		virtual void OnMouseUp(const std::unique_ptr<MouseEventArgs>& EventArgs);

	protected:
		// Get custom control creation parameters for this instance.
		virtual CreateParams GetCreateParams();

	private:
		// Internal cached dialog result that this button represents
		DialogResult _DialogResult;
	};
}