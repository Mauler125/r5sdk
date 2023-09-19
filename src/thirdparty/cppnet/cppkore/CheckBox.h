#pragma once

#include <cstdint>
#include "Control.h"
#include "Appearence.h"
#include "ButtonBase.h"
#include "CheckState.h"
#include "ContentAlignment.h"

namespace Forms
{
	// Represents a Windows check box.
	class CheckBox : public ButtonBase
	{
	public:
		CheckBox();
		virtual ~CheckBox() = default;

		// Gets the value that determines the appearance of a check box control.
		Appearence GetAppearence();
		// Sets the value that determines the appearance of a check box control.
		void SetAppearence(Appearence Value);

		// Gets a value indicating whether the check box automatically checks itself.
		bool AutoCheck();
		// Sets a value indicating whether the check box automatically checks itself.
		void SetAutoCheck(bool Value);

		// Gets a value indicating whether the check box is checked.
		bool Checked();
		// Sets a value indicating whether the check box is checked.
		void SetChecked(bool Value);

		// Gets a value indicating whether the check box is checked.
		CheckState GetCheckState();
		// Sets a value indicating whether the check box is checked.
		void SetCheckState(CheckState Value);

		// Gets a value indicating whether the check box will allow three check states rather than two.
		bool ThreeState();
		// Sets a value indicating whether the check box will allow three check states rather than two.
		void SetThreeState(bool Value);

		// We must define base events here
		virtual void OnClick();
		virtual void OnAppearenceChanged();
		virtual void OnCheckedChanged();
		virtual void OnCheckStateChanged();
		virtual void OnHandleCreated();
		virtual void OnMouseUp(const std::unique_ptr<MouseEventArgs>& EventArgs);

		// We must define event handlers here
		EventBase<void(*)(Control*)> AppearenceChanged;
		EventBase<void(*)(Control*)> CheckedChanged;
		EventBase<void(*)(Control*)> CheckStateChanged;

	protected:
		// Get custom control creation parameters for this instance.
		virtual CreateParams GetCreateParams();

	private:
		// Whether or not the control handles checking itself.
		bool _AutoCheck;
		// Whether or not the control handles three checked states.
		bool _ThreeState;

		// The appearence of the control.
		Appearence _Appearence;
		// The checked state of the control.
		CheckState _CheckState;
	};
}