#pragma once

#include <cstdint>
#include <type_traits>

namespace Forms
{
	// This enumeration represents the ButtonBase flags...
	enum class ButtonFlags
	{
		FlagMouseOver = 0x0001,
		FlagMouseDown = 0x0002,
		FlagMousePressed = 0x0004,
		FlagInButtonUp = 0x0008,
		FlagCurrentlyAnimating = 0x0010,
		FlagAutoEllipsis = 0x0020,
		FlagIsDefault = 0x0040,
		FlagUseMnemonic = 0x0080,
		FlagShowToolTip = 0x0100,
	};

	//
	// Allow bitwise operations on this enumeration
	//
	constexpr ButtonFlags operator|(ButtonFlags Lhs, ButtonFlags Rhs)
	{
		return static_cast<ButtonFlags>(static_cast<std::underlying_type<ButtonFlags>::type>(Lhs) | static_cast<std::underlying_type<ButtonFlags>::type>(Rhs));
	};
}