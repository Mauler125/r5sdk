#pragma once

#include <cstdint>
#include "ConsoleKey.h"

namespace System
{
	// Control key modifiers
	enum class ConsoleModifiers : uint8_t
	{
		None = 0,
		Alt = 1,
		Shift = 2,
		Control = 4
	};

	// Information about the current keystroke
	struct ConsoleKeyInfo
	{
		char KeyChar;
		ConsoleKey Key;
		ConsoleModifiers Modifiers;

		ConsoleKeyInfo(char kChar, ConsoleKey Key, bool Shift, bool Alt, bool Control)
			: KeyChar(kChar), Key(Key), Modifiers(ConsoleModifiers::None)
		{
			if (Shift)
				*(uint8_t*)&Modifiers |= (uint8_t)ConsoleModifiers::Shift;
			if (Alt)
				*(uint8_t*)&Modifiers |= (uint8_t)ConsoleModifiers::Alt;
			if (Control)
				*(uint8_t*)&Modifiers |= (uint8_t)ConsoleModifiers::Control;
		}
	};
}