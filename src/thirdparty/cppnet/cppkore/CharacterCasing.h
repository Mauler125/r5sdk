#pragma once

#include <cstdint>

namespace Forms
{
	// Specifies the case of characters in a Textbox control.
	enum class CharacterCasing
	{
		// The case of characters is left unchanged.
		Normal = 0,
		// Converts all characters to uppercase.
		Upper = 1,
		// Converts all characters to lowercase.
		Lower = 2,
	};
}