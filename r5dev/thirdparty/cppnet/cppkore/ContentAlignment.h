#pragma once

#include <cstdint>

namespace Drawing
{
	// Specifies alignment of content on the drawing surface.
	enum class ContentAlignment
	{
		// Content is vertically aligned at the top, and horizontally
		// aligned on the left.
		TopLeft = 0x001,
		// Content is vertically aligned at the top, and
		// horizontally aligned at the center.
		TopCenter = 0x002,
		// Content is vertically aligned at the top, and
		// horizontally aligned on the right.
		TopRight = 0x004,
		// Content is vertically aligned in the middle, and
		// horizontally aligned on the left.
		MiddleLeft = 0x010,
		// Content is vertically aligned in the middle, and
		// horizontally aligned at the center.
		MiddleCenter = 0x020,
		// Content is vertically aligned in the middle, and horizontally aligned on the
		// right.
		MiddleRight = 0x040,
		// Content is vertically aligned at the bottom, and horizontally aligned on the
		// left.
		BottomLeft = 0x100,
		// Content is vertically aligned at the bottom, and horizontally aligned at the
		// center.
		BottomCenter = 0x200,
		// Content is vertically aligned at the bottom, and horizontally aligned on the right.
		BottomRight = 0x400,
	};

	//
	// Allow bitwise operations on this enumeration
	//
	constexpr ContentAlignment operator|(ContentAlignment Lhs, ContentAlignment Rhs)
	{
		return static_cast<ContentAlignment>(static_cast<std::underlying_type<ContentAlignment>::type>(Lhs) | static_cast<std::underlying_type<ContentAlignment>::type>(Rhs));
	};

	//
	// Content alignment masks
	//

	constexpr static ContentAlignment AnyRightAlign = ContentAlignment::TopRight | ContentAlignment::MiddleRight | ContentAlignment::BottomRight;
	constexpr static ContentAlignment AnyLeftAlign = ContentAlignment::TopLeft | ContentAlignment::MiddleLeft | ContentAlignment::BottomLeft;
	constexpr static ContentAlignment AnyTopAlign = ContentAlignment::TopLeft | ContentAlignment::TopCenter | ContentAlignment::TopRight;
	constexpr static ContentAlignment AnyBottomAlign = ContentAlignment::BottomLeft | ContentAlignment::BottomCenter | ContentAlignment::BottomRight;
	constexpr static ContentAlignment AnyMiddleAlign = ContentAlignment::MiddleLeft | ContentAlignment::MiddleCenter | ContentAlignment::MiddleRight;
	constexpr static ContentAlignment AnyCenterAlign = ContentAlignment::TopCenter | ContentAlignment::MiddleCenter | ContentAlignment::BottomCenter;
}