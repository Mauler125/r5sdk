#pragma once

#include <cstdint>
#include <WinUser.h>

namespace Drawing
{
	// Defines options for drawing/measuring text in TextRenderer.
	enum class TextFormatFlags : uint32_t
	{
		Bottom = DT_BOTTOM,
		EndEllipsis = DT_END_ELLIPSIS,
		ExpandTabs = DT_EXPANDTABS,
		ExternalLeading = DT_EXTERNALLEADING,
		Default = DT_TOP | DT_LEFT,
		HidePrefix = DT_HIDEPREFIX,
		HorizontalCenter = DT_CENTER,
		Internal = DT_INTERNAL,
		Left = DT_LEFT,
		ModifyString = DT_MODIFYSTRING,
		NoClipping = DT_NOCLIP,
		NoPrefix = DT_NOPREFIX,
		NoFullWidthCharacterBreak = DT_NOFULLWIDTHCHARBREAK,
		PathEllipsis = DT_PATH_ELLIPSIS,
		PrefixOnly = DT_PREFIXONLY,
		Right = DT_RIGHT,
		RightToLeft = DT_RTLREADING,
		SingleLine = DT_SINGLELINE,
		TextBoxControl = DT_EDITCONTROL,
		Top = DT_TOP,
		VerticalCenter = DT_VCENTER,
		WordBreak = DT_WORDBREAK,
		WordEllipsis = DT_WORD_ELLIPSIS,
	};

	//
	// Allow bitwise operations on this enumeration
	//
	constexpr TextFormatFlags operator|(TextFormatFlags Lhs, TextFormatFlags Rhs)
	{
		return static_cast<TextFormatFlags>(static_cast<std::underlying_type<TextFormatFlags>::type>(Lhs) | static_cast<std::underlying_type<TextFormatFlags>::type>(Rhs));
	};
	constexpr TextFormatFlags& operator|=(TextFormatFlags& Lhs, const TextFormatFlags& Rhs)
	{
		Lhs = static_cast<TextFormatFlags>(static_cast<std::underlying_type<TextFormatFlags>::type>(Lhs) | static_cast<std::underlying_type<TextFormatFlags>::type>(Rhs));
		return Lhs;
	};
}