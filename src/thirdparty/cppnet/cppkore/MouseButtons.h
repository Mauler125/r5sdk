#pragma once

#include <cstdint>

namespace Forms
{
	// Specifies constants that define which mouse button was pressed.
	enum class MouseButtons
	{
		// The left mouse button was pressed.
		Left = 0x00100000,
		// No mouse button was pressed.
		None = 0x00000000,
		// The right mouse button was pressed.
		Right = 0x00200000,
		// The middle mouse button was pressed.
		Middle = 0x00400000,
		// [To be supplied.]
		XButton1 = 0x00800000,
		// [To be supplied.]
		XButton2 = 0x01000000,
	};

	//
	// Allow bitwise operations on this enumeration
	//
	constexpr MouseButtons operator|(MouseButtons Lhs, MouseButtons Rhs)
	{
		return static_cast<MouseButtons>(static_cast<std::underlying_type<MouseButtons>::type>(Lhs) | static_cast<std::underlying_type<MouseButtons>::type>(Rhs));
	};
}