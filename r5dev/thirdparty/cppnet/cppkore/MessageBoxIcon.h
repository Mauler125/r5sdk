#pragma once

#include <cstdint>

namespace Forms
{
	// This enumeration represents the possible message box icons.
	enum class MessageBoxIcon
	{
		// Specifies that the message box contain no symbols.
		None = 0,
		// Specifies that the message box contains a hand symbol.
		Hand = 0x00000010,
		// Specifies that the message box contains a question mark symbol.
		Question = 0x00000020,
		// Specifies that the message box contains an exclamation symbol.
		Exclamation = 0x00000030,
		// Specifies that the message box contains an asterisk symbol.
		Asterisk = 0x00000040,
		// Specifies that the message box contains a hand icon.
		Stop = Hand,
		// Specifies that the message box contains a hand icon.
		Error = Hand,
		// Specifies that the message box contains an exclamation icon.
		Warning = Exclamation,
		// Specifies that the message box contains an asterisk icon.
		Information = Asterisk,
	};
}