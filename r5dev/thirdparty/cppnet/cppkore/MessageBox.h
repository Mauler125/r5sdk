#pragma once

#include <cstdint>
#include <memory>
#include "Control.h"
#include "StringBase.h"
#include "DialogResult.h"
#include "MessageBoxIcon.h"
#include "MessageBoxButtons.h"
#include "MessageBoxOptions.h"
#include "MessageBoxDefaultButton.h"

// Remove built-in macros that interfere with the api
#undef MessageBox

namespace Forms
{
	// Displays a message box that can contain text, buttons, and symbols that
	// inform and instruct the user.
	class MessageBox
	{
	public:
		// Displays a message box with specified text.
		static DialogResult Show(const string& Text);
		// Displays a message box with specified text and caption.
		static DialogResult Show(const string& Text, const string& Caption);
		// Displays a message box with specified text, caption, and style.
		static DialogResult Show(const string& Text, const string& Caption, MessageBoxButtons Buttons);
		// Displays a message box with specified text, caption, and style.
		static DialogResult Show(const string& Text, const string& Caption, MessageBoxButtons Buttons, MessageBoxIcon Icon);
		// Displays a message box with specified text, caption, and style.
		static DialogResult Show(Control* Owner, const string& Text, const string& Caption, MessageBoxButtons Buttons, MessageBoxIcon Icon);
		// Displays a message box with specified text, caption, and style.
		static DialogResult Show(Control* Owner, const string& Text, const string& Caption, MessageBoxButtons Buttons, MessageBoxIcon Icon, MessageBoxDefaultButton DefaultButton);
		// Displays a message box with specified text, caption, and style.
		static DialogResult Show(Control* Owner, const string& Text, const string& Caption, MessageBoxButtons Buttons, MessageBoxIcon Icon, MessageBoxDefaultButton DefaultButton, MessageBoxOptions Options);

		// Sets user defined message box colors for Foreground, Background, and Dialog Bottom Strip.
		static void SetMessageBoxColors(Drawing::Color Foreground = Drawing::Color::Black, Drawing::Color Background = Drawing::Color::White, Drawing::Color Bottom = Drawing::Color(240, 240, 240));

	private:
		// Converts a native windows result to a dialog result
		static DialogResult Win32ToDialogResult(uint32_t Value);
		// Handles message box logic
		static DialogResult ShowCore(const string& Text, const string& Caption, MessageBoxButtons Buttons, MessageBoxIcon Icon, MessageBoxDefaultButton DefaultButton, MessageBoxOptions Options, bool ShowHelp, Control* Owner);

		// Internal help button constant
		constexpr static uint32_t HELP_BUTTON = 0x00004000;
	};
}