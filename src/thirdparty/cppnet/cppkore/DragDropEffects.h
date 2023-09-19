#pragma once

#include <cstdint>
#include <type_traits>

namespace Forms
{
	// Specifies the effects of a drag-and-drop operation.
	enum class DragDropEffects
	{
		None = 0x0,
		Copy = 0x1,
		Move = 0x2,
		Link = 0x4,
		Scroll = (int)0x80000000,
		All = Copy | Move | Scroll
	};

	//
	// Allow bitwise operations on this enumeration
	//
	constexpr DragDropEffects operator|(DragDropEffects Lhs, DragDropEffects Rhs)
	{
		return static_cast<DragDropEffects>(static_cast<std::underlying_type<DragDropEffects>::type>(Lhs) | static_cast<std::underlying_type<DragDropEffects>::type>(Rhs));
	};
}