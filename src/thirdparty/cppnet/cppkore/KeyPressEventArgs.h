#pragma once

#include <memory>

namespace Forms
{
	class KeyPressEventArgs
	{
	public:
		KeyPressEventArgs() = default;
		KeyPressEventArgs(char KeyChar);
		~KeyPressEventArgs() = default;

		// Gets or sets the character corresponding to the key pressed.
		char KeyChar;

		// Gets a value indicating whether the event was handled.
		bool Handled();
		// Sets a value indicating whether the event was handled.
		void SetHandled(bool Value);

	private:
		// Internal key data from the event
		bool _Handled;
	};
}