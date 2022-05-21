#pragma once

#include <cstdint>
#include "Control.h"
#include "TextBoxBase.h"
#include "ScrollBars.h"
#include "CharacterCasing.h"
#include "HorizontalAlignment.h"

namespace Forms
{
	// Represents a Windows text box control.
	class TextBox : public TextBoxBase
	{
	public:
		TextBox();
		virtual ~TextBox() = default;

		// Gets a value indicating whether pressing ENTER
		// in a multiline TextBox control creates a new line of text
		// in the control or activates the default button for the form.
		bool AcceptsReturn();
		// Sets a value indicating whether pressing ENTER
		// in a multiline TextBox control creates a new line of text
		// in the control or activates the default button for the form.
		void AcceptsReturn(bool Value);

		// Gets whether the TextBox control
		// modifies the case of characters as they are typed.
		CharacterCasing GetCharacterCasing();
		// Sets whether the TextBox control
		// modifies the case of characters as they are typed.
		void SetCharacterCasing(CharacterCasing Value);

		// Determines if the control is in password protect mode.
		virtual bool PasswordProtect();

		// Gets the character used to mask characters in a single-line text box.
		char PasswordChar();
		// Sets the character used to mask characters in a single-line text box.
		void SetPasswordChar(char Value);

		// Gets which scroll bars should
		// appear in a multiline TextBox control.
		ScrollBars GetScrollBars();
		// Sets which scroll bars should
		// appear in a multiline TextBox control.
		void SetScrollBars(ScrollBars Value);

		// Sets the current text in the text box.
		virtual void SetText(const string& Value);

		// Gets how text is aligned in a TextBox control.
		HorizontalAlignment TextAlign();
		// Sets how text is aligned in a TextBox control.
		void SetTextAlign(HorizontalAlignment Value);

		// Gets if the text in the edit control should appear as
		// the default password character.
		bool UseSystemPasswordChar();
		// Sets if the text in the edit control should appear as
		// the default password character.
		void SetUseSystemPasswordChar(bool Value);

		// We must define base events here
		virtual void OnSizeChanged();
		virtual void OnHandleCreated();
		virtual void OnGotFocus();

		// The standard windows message pump for this control.
		virtual void WndProc(Message& Msg);

	protected:
		// Get custom control creation parameters for this instance.
		virtual CreateParams GetCreateParams();

	private:
		// Internal cached flags
		bool _AcceptsReturn;
		char _PasswordChar;
		bool _UseSystemPasswordChar;
		CharacterCasing _CharacterCasing;
		ScrollBars _Scrollbars;
		HorizontalAlignment _TextAlign;
		bool _SelectionSet;

		// Used for vertical centering of text in non-multiline mode
		RECT _NCRectTop;
		RECT _NCRectBottom;
		RECT _NCRectLeft;
		RECT _NCRectRight;
		bool _SizeRectDirty;
		bool _IsCalcRects;

		// We must define each window message handler here...
		void WmNcCalcSize(Message& Msg);
		void WmNcPaint(Message& Msg);

		// Internal routine to calculate size rects for the control
		void CalculateSizeRects();
	};
}