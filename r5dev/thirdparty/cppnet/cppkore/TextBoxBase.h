#pragma once
#pragma once

#include <cstdint>
#include "StringBase.h"
#include "ListBase.h"
#include "Control.h"
#include "FlatStyle.h"
#include "BorderStyle.h"
#include "TextBoxFlags.h"
#include "ContentAlignment.h"

namespace Forms
{
	// Implements the basic functionality required by text controls.
	class TextBoxBase : public Control
	{
	public:
		virtual ~TextBoxBase() = default;

		// Gets the border style appearence of the text box control.
		BorderStyle GetBorderStyle();
		// Sets the border style appearence of the text box control.
		void SetBorderStyle(BorderStyle Value);

		// Gets a value indicating whether pressing the TAB key
		// in a multiline text box control types
		// a TAB character in the control instead of moving the focus to the next control
		// in the tab order.
		bool AcceptsTab();
		// Sets a value indicating whether pressing the TAB key
		// in a multiline text box control types
		// a TAB character in the control instead of moving the focus to the next control
		// in the tab order.
		void SetAcceptsTab(bool Value);

		// Gets a value indicating whether the user can undo the previous operation in a text box control.
		bool CanUndo();

		// Gets a value indicating whether the selected
		// text in the text box control remains highlighted when the control loses focus.
		bool HideSelection();
		// Sets a value indicating whether the selected
		// text in the text box control remains highlighted when the control loses focus.
		void SetHideSelection(bool Value);

		// Gets the lines of text in an text box control.
		List<string> Lines();
		// Sets the lines of text in an text box control.
		void SetLines(const List<string>& Value);

		// Gets the maximum number of characters the user can type into the text box control.
		virtual uint32_t MaxLength();
		// Sets the maximum number of characters the user can type into the text box control.
		virtual void SetMaxLength(uint32_t Value);

		// Gets a value that indicates that the text box control has been modified by the user since
		// the control was created or its contents were last set.
		bool Modified();
		// Sets a value that indicates that the text box control has been modified by the user since
		// the control was created or its contents were last set.
		void SetModified(bool Value);

		// Gets a value indicating whether this is a multiline text box control.
		virtual bool Multiline();
		// Sets a value indicating whether this is a multiline text box control.
		virtual void SetMultiline(bool Value);

		// Determines if the control is in password protect mode.
		virtual bool PasswordProtect();

		// Gets a value indicating whether text in the text box is read-only.
		bool ReadOnly();
		// Sets a value indicating whether text in the text box is read-only.
		void SetReadOnly(bool Value);

		// Gets the currently selected text in the control.
		virtual string SelectedText();
		// Sets the currently selected text in the control.
		virtual void SetSelectedText(const string& Value);

		// Gets the number of characters selected in the text box.
		virtual int32_t SelectionLength();
		// Sets the number of characters selected in the text box.
		virtual void SetSelectionLength(int32_t Value);

		// Gets the starting point of text selected in the text box.
		int32_t SelectionStart();
		// Sets the starting point of text selected in the text box.
		void SetSelectionStart(int32_t Value);

		// Gets the current text in the text box.
		virtual string Text();
		// Sets the current text in the text box.
		virtual void SetText(const string& Value);

		// Gets the length of the text in the control.
		virtual uint32_t TextLength();

		// Gets a value indicating whether a text box will wrap it's contents.
		bool WordWrap();
		// Sets a value indicating whether a text box will wrap it's contents.
		void SetWordWrap(bool Value);

		// Append text to the current text of the text box.
		void AppendText(const string& Text);

		// Clears all the text from the text box control.
		void Clear();
		// Clears information about the most recent operation
		// from the undo buffer of the text box.
		void ClearUndo();

		// Copies the current selection in the text box to the Clipboard.
		void Copy();
		// Moves the current selection in the text box to the Clipboard.
		void Cut();
		// Replaces the current selection in the text box with the contents of the Clipboard.
		void Paste();
		// Undoes the last edit operation in the text box.
		void Undo();

		// Selects a range of text in the text box.
		void Select(int32_t Start, int32_t Length);
		// Selects all text in the text box.
		void SelectAll();
		// Sets the SelectionLength to 0.
		void DeselectAll();

		// Ensures that the caret is visible in the text box window.
		void ScrollToCaret();

		// We must define control event bases here
		virtual void OnMouseUp(const std::unique_ptr<MouseEventArgs>& EventArgs);
		virtual void OnHideSelectionChanged();
		virtual void OnModifiedChanged();
		virtual void OnMultilineChanged();
		virtual void OnReadOnlyChanged();
		virtual void OnAcceptsTabChanged();
		virtual void OnHandleCreated();
		virtual void OnHandleDestroyed();

		// We must define event handlers here
		EventBase<void(*)(Control*)> HideSelectionChanged;
		EventBase<void(*)(Control*)> ModifiedChanged;
		EventBase<void(*)(Control*)> MultilineChanged;
		EventBase<void(*)(Control*)> ReadOnlyChanged;
		EventBase<void(*)(Control*)> AcceptsTabChanged;

		// The standard windows message pump for this control.
		virtual void WndProc(Message& Msg);

	protected:
		TextBoxBase();

		// Get custom control creation parameters for this instance.
		virtual CreateParams GetCreateParams();

		// Sets the current text of the Window
		virtual void SetWindowText(const string& Value);

		// Whether or not this control can raise the text changed event.
		virtual bool CanRaiseTextChangedEvent();
		// Updates the controls max length property.
		virtual void UpdateMaxLength();
		// Internal routine to set the selected text.
		virtual void SetSelectedTextInternal(const string& Text, bool ClearUndo);
		// Internal routine to perform actual selection.
		virtual void SelectInternal(int32_t Start, int32_t Length, int32_t TextLen);

		// Returns the end position (One past the last char).
		int32_t GetEndPosition();

		// Calculates the start and length of the selection.
		void GetSelectionStartAndLength(int32_t& Start, int32_t& Length);
		// Calculates the proper start and end of the selection.
		void AdjustSelectionStartAndEnd(int32_t SelStart, int32_t SelLength, int32_t& Start, int32_t& End, int32_t TextLen);

		// Gets edit specific flags
		bool GetFlag(TextBoxFlags Flag);
		// Sets edit specific flags
		void SetFlag(TextBoxFlags Flags, bool Value);

		// Control specific flags
		TextBoxFlags _Flags;
		// Controls the style of the control border
		BorderStyle _BorderStyle;
		// The maximum length in characters
		uint32_t _MaxLength;

		// Used to set the selection before the control is initialized
		int32_t _SelectionStart;
		int32_t _SelectionLength;
		bool _DoubleClickFired;

	private:
		// We must define each window message handler here...
		void WmReflectCommand(Message& Msg);
		void WmGetDlgCode(Message& Msg);
		void WmSetFont(Message& Msg);
	};
}