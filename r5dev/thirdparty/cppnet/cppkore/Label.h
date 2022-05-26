#pragma once

#include <cstdint>
#include "Control.h"
#include "FlatStyle.h"
#include "BorderStyle.h"
#include "ContentAlignment.h"

namespace Forms
{
	// Represents a standard Windows label.
	class Label : public Control
	{
	public:
		Label();
		virtual ~Label() = default;

		// Gets the drawing mode of the label control.
		bool OwnerDraw();
		// Sets the drawing mode of the label control.
		void SetOwnerDraw(bool Value);

		// Gets the alignment of the text on the label control.
		Drawing::ContentAlignment TextAlign();
		// Sets the alignment of the text on the label control.
		void SetTextAlign(Drawing::ContentAlignment Value);

		// Gets the flat style appearance of the label control.
		FlatStyle GetFlatStyle();
		// Sets the flat style appearance of the label control.
		void SetFlatStyle(FlatStyle Value);

		// Gets the border style appearence of the label control.
		BorderStyle GetBorderStyle();
		// Sets the border style appearence of the label control.
		void SetBorderStyle(BorderStyle Value);

		// Override WndProc for specific label messages.
		virtual void WndProc(Message& Msg);

	protected:
		// Get custom control creation parameters for this instance.
		virtual CreateParams GetCreateParams();

	private:
		// Whether or not the control will draw itself
		bool _OwnerDraw;
		// Controls the style of the control
		FlatStyle _FlatStyle;
		// Controls the style of the control border
		BorderStyle _BorderStyle;
		// Controls the alignment of text on the control
		Drawing::ContentAlignment _TextAlign;
	};
}