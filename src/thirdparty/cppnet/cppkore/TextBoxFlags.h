#pragma once

#include <cstdint>
#include <type_traits>

namespace Forms
{
	// This enumeration represents the TextBoxBase flags...
	enum class TextBoxFlags
	{
		FlagAutoSize = 0x00000001,
		FlagHideSelection = 0x00000002,
		FlagMultiline = 0x00000004,
		FlagModified = 0x00000010,
		FlagReadOnly = 0x00000020,
		FlagAcceptsTab = 0x00000040,
		FlagWordWrap = 0x00000080,
		FlagCreatingHandle = 0x00000100,
		FlagCodeUpdateText = 0x00000200,
		FlagShortcutsEnabled = 0x00000400,
		FlagScrollToCaretOnHandleCreated = 0x00000800,
		FlagSetSelectionOnHandleCreated = 0x00001000,
	};

	//
	// Allow bitwise operations on this enumeration
	//
	constexpr TextBoxFlags operator|(TextBoxFlags Lhs, TextBoxFlags Rhs)
	{
		return static_cast<TextBoxFlags>(static_cast<std::underlying_type<TextBoxFlags>::type>(Lhs) | static_cast<std::underlying_type<TextBoxFlags>::type>(Rhs));
	};
}