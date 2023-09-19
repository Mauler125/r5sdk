#pragma once

#include <memory>
#include "Keys.h"

namespace Forms
{
	// Provides data for the OnKey events.
	class KeyEventArgs
	{
	public:
		KeyEventArgs() = default;
		KeyEventArgs(Keys KeyData);
		~KeyEventArgs() = default;

		// Gets a value indicating whether the ALT key was pressed.
		bool Alt();
		// Gets a value indicating whether the CTRL key was pressed.
		bool Control();
		// Gets a value indicating whether the SHIFT key was pressed.
		bool Shift();

		// Gets a value indicating whether the event was handled.
		bool Handled();
		// Sets a value indicating whether the event was handled.
		void SetHandled(bool Value);

		// Gets a value indicating whether the key press should be surpressed.
		bool SuppressKeyPress();
		// Sets a value indicating whether the key press should be surpressed.
		void SetSurpressKeyPress(bool Value);

		// Gets the keyboard code for a OnKeyDown or OnKeyUp event.
		Keys KeyCode();
		// Gets the keyboard value for a OnKeyDown or OnKeyUp event.
		uint32_t KeyValue();
		// Gets the key data for a OnKeyDown or OnKeyUp event.
		Keys KeyData();
		// This returns which modifier keys (CTRL, SHIFT, and/or ALT) were pressed.
		Keys Modifiers();

	private:
		// Internal key data from the event
		Keys _KeyData;
		bool _SuppressKeyPress;
		bool _Handled;
	};
}