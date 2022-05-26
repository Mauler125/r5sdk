#pragma once

#include <cstdint>
#include <type_traits>

namespace Forms
{
	// Specifies how a control anchors to the edges of its container.
	enum class AnchorStyles
	{
		// The control is anchored to the top edge of its container.
		Top = 0x1,
		// The control is anchored to the bottom edge of its container.
		Bottom = 0x2,
		// The control is anchored to the left edge of its container.
		Left = 0x4,
		// The control is anchored to the right edge of its container.
		Right = 0x8,
		// The control is not anchored to any edges of its container.
		None = 0x0
	};

	//
	// Allow bitwise operations on this enumeration
	//
	constexpr AnchorStyles operator|(AnchorStyles Lhs, AnchorStyles Rhs)
	{
		return static_cast<AnchorStyles>(static_cast<std::underlying_type<AnchorStyles>::type>(Lhs) | static_cast<std::underlying_type<AnchorStyles>::type>(Rhs));
	};
}