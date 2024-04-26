#pragma once

#include <cstdint>
#include "Control.h"
#include "StringBase.h"
#include "ListBase.h"
#include "ImmutableStringBase.h"
#include "DrawMode.h"
#include "FlatStyle.h"
#include "ComboBoxStyle.h"

namespace Forms
{
	// Displays an editing field and a list control.
	class ComboBox : public Control
	{
	public:
		ComboBox();
		virtual ~ComboBox() = default;

		// Gets whether the control is drawn by Windows or by the user.
		DrawMode GetDrawMode();
		// Sets whether the control is drawn by Windows or by the user.
		void SetDrawMode(DrawMode Value);

		// Gets the width of the drop down box in a combo box.
		uint32_t DropDownWidth();
		// Sets the width of the drop down box in a combo box.
		void SetDropDownWidth(uint32_t Value);

		// Gets the Height of the drop down box in a combo box.
		uint32_t DropDownHeight();
		// Sets the Height of the drop down box in a combo box.
		void SetDropDownHeight(uint32_t Value);

		// Indicates whether the DropDown of the combo is  currently dropped down.
		bool DroppedDown();
		// Sets whether the DropDown of the combo is  currently dropped down.
		void SetDroppedDown(bool Value);

		// Gets the flat style appearance of the button control.
		FlatStyle GetFlatStyle();
		// Sets the flat style appearance of the button control.
		void SetFlatStyle(FlatStyle Value);

		// Gets if the combo should avoid showing partial Items.
		bool IntegralHeight();
		// Sets if the combo should avoid showing partial Items.
		void SetIntegralHeight(bool Value);

		// Gets the height of an item in the combo box.
		int32_t ItemHeight();
		// Sets the height of an item in the combo box.
		void SetItemHeight(int32_t Value);

		// Gets the maximum number of items to be shown in the dropdown portion
		// of the ComboBox. This number can be between 1 and 100.
		uint8_t MaxDropDownItems();
		// Sets the maximum number of items to be shown in the dropdown portion
		// of the ComboBox. This number can be between 1 and 100.
		void SetMaxDropDownItems(uint8_t Value);

		// Gets the maximum length of the text the user may type into the edit control of a combo box.
		uint32_t MaxLength();
		// Sets the maximum length of the text the user may type into the edit control of a combo box.
		void SetMaxLength(uint32_t Value);

		// Gets the [zero based] index of the currently selected item in the combos list.
		// Note If the value of index is -1, then the ComboBox is
		// set to have no selection.
		int32_t SelectedIndex();
		// Sets the [zero based] index of the currently selected item in the combos list.
		// Note If the value of index is -1, then the ComboBox is
		// set to have no selection.
		void SetSelectedIndex(int32_t Value);

		// Gets the selected text in the edit component of the ComboBox.
		String SelectedText();
		// Sets the selected text in the edit component of the ComboBox.
		void SetSelectedText(const String& Value);

		// Gets length, in characters, of the selection in the editbox.
		int32_t SelectionLength();
		// Sets length, in characters, of the selection in the editbox.
		void SetSelectionLength(int32_t Value);

		// Gets the [zero-based] index of the first character in the current text selection.
		int32_t SelectionStart();
		// Sets the [zero-based] index of the first character in the current text selection.
		void SetSelectionStart(int32_t Value);

		// Gets the type of combo that we are right now.
		ComboBoxStyle DropDownStyle();
		// Sets the type of combo that we are right now.
		void SetDropDownStyle(ComboBoxStyle Value);

		// Selects the text in the editable portion of the ComboBox at the
		void Select(int32_t Start, int32_t Length);

		// The items contained in the combo box.
		struct ComboBoxItemCollection
		{
			ComboBoxItemCollection(ComboBox* Owner);
			~ComboBoxItemCollection() = default;

			// Adds an item to the combo box.
			void Add(const imstring& Value);
			// Inserts an item to the combo box.
			void Insert(int32_t Index, const imstring& Value);
			// Removes all items from the combo box.
			void Clear();
			// Checks if the combo box contains the value.
			bool Contains(const imstring& Value);
			// Gets the index of the item, if any.
			int32_t IndexOf(const imstring& Value);
			// Removes an item at the specified index.
			void RemoveAt(int32_t Index);
			// Removes an item from the combo box.
			void Remove(const imstring& Value);
			// Gets the count of items in the combo box.
			uint32_t Count();

			// Iterater classes
			String* begin() const;
			String* end() const;

		protected:
			// Internal references
			ComboBox* _Owner;
			List<String> _Items;
		} Items;

		// We must define control event bases here
		virtual void OnHandleCreated();
		virtual void OnSelectedItemChanged();
		virtual void OnSelectedIndexChanged();
		virtual void OnDropDownOpened();
		virtual void OnDropDownClosed();

		// We must define event handlers here
		EventBase<void(*)(Control*)> SelectedItemChanged;
		EventBase<void(*)(Control*)> SelectedIndexChanged;
		EventBase<void(*)(Control*)> DropDownOpened;
		EventBase<void(*)(Control*)> DropDownClosed;

		// Override WndProc for specific combo box messages.
		virtual void WndProc(Message& Msg);

	protected:
		// Get custom control creation parameters for this instance.
		virtual CreateParams GetCreateParams();

		// Sets the text and background colors of the DC, and returns the background HBRUSH.
		virtual uintptr_t InitializeDCForWmCtlColor(HDC Dc, int32_t Message);

		// Internal routine to add an item.
		int32_t NativeAdd(const char* Value);
		// Internal routine to clear the items.
		void NativeClear();
		// Internal routine to insert an item.
		int32_t NativeInsert(int32_t Index, const char* Value);
		// Internal routine to remove an item.
		void NativeRemoveAt(int32_t Index);

	private:
		// Internal cached flags
		FlatStyle _FlatStyle;
		DrawMode _DrawMode;
		ComboBoxStyle _DropDownStyle;

		// Internal show partial items
		bool _IntegralHeight;

		// Internal size of dropdown
		int32_t _DropDownWidth;
		int32_t _DropDownHeight;
		int32_t _SelectedIndex;

		// Internal children
		HWND _ChildEdit;
		HWND _ChildListBox;

		// Internal maximum items
		uint8_t _MaxDropDownItems;

		// Internal maximun length
		uint32_t _MaximumLength;

		// We must define each window message handler here...
		void WmReflectCommand(Message& Msg);
	};
}