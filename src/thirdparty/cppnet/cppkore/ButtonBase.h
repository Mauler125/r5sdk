#pragma once

#include <cstdint>
#include "Control.h"
#include "FlatStyle.h"
#include "ButtonFlags.h"
#include "ContentAlignment.h"

namespace Forms
{
	// Implements the basic functionality required by a button control.
	class ButtonBase : public Control
	{
	public:
		virtual ~ButtonBase() = default;

		// Gets the drawing mode of the button control.
		bool OwnerDraw();
		// Sets the drawing mode of the button control.
		void SetOwnerDraw(bool Value);

		// Gets the alignment of the text on the button control.
		Drawing::ContentAlignment TextAlign();
		// Sets the alignment of the text on the button control.
		void SetTextAlign(Drawing::ContentAlignment Value);

		// Gets the flat style appearance of the button control.
		FlatStyle GetFlatStyle();
		// Sets the flat style appearance of the button control.
		void SetFlatStyle(FlatStyle Value);

		// Get whether or not this control is the default response.
		bool IsDefault();
		// Sets whether or not this control is the default response.
		void SetIsDefault(bool Value);

		// We must define base events here
		virtual void OnLostFocus();
		virtual void OnGotFocus();
		virtual void OnMouseEnter();
		virtual void OnMouseLeave();
		virtual void OnEnabledChanged();
		virtual void OnTextChanged();
		virtual void OnMouseMove(const std::unique_ptr<MouseEventArgs>& EventArgs);
		virtual void OnMouseDown(const std::unique_ptr<MouseEventArgs>& EventArgs);
		virtual void OnMouseUp(const std::unique_ptr<MouseEventArgs>& EventArgs);
		virtual void OnKeyUp(const std::unique_ptr<KeyEventArgs>& EventArgs);
		virtual void OnKeyDown(const std::unique_ptr<KeyEventArgs>& EventArgs);

		// Override WndProc for specific button messages.
		virtual void WndProc(Message& Msg);

	protected:
		ButtonBase();

		// Get custom control creation parameters for this instance.
		virtual CreateParams GetCreateParams();

		// Used for quick re-painting of the button after the pressed state.
		void ResetFlagsAndPaint();

		// Gets button specific flags
		bool GetFlag(ButtonFlags Flag);
		// Sets button specific flags
		void SetFlag(ButtonFlags Flags, bool Value);

		// Whether or not the control will draw itself
		bool _OwnerDraw;
		// Control specific flags
		ButtonFlags _Flags;
		// Controls the style of the control
		FlatStyle _FlatStyle;
		// Controls the alignment of text on the control
		Drawing::ContentAlignment _TextAlign;
	};
}