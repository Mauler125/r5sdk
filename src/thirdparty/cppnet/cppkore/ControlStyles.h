#pragma once

#include <cstdint>
#include <type_traits>

namespace Forms
{
	// Specifies control functionality.
	enum class ControlStyles
	{
		//  Indicates whether the control is a container-like control.
		ContainerControl = 0x00000001,
		// The control paints itself; WM_PAINT and WM_ERASEBKGND messages are not passed 
		// on to the underlying NativeWindow.
		UserPaint = 0x00000002,
		//  If specified, a PaintBackground event will not be raised, OnPaintBackground will not be called,
		// and Invalidate() will not invalidate the background of the HWND.
		Opaque = 0x00000004,
		// The control is completely redrawn when it is resized.
		ResizeRedraw = 0x00000010,
		// The control has a fixed width.
		FixedWidth = 0x00000020,
		// The control has a fixed height.
		FixedHeight = 0x00000040,
		// If set, windows forms calls OnClick and raises the Click event when the control is clicked
		// (unless it's the second click of a double-click and StandardDoubleClick is specified).
		// Regardless of this style, the control may call OnClick directly.
		StandardClick = 0x00000100,
		//  The control can get the focus.
		Selectable = 0x00000200,
		// The control does its own mouse processing; WM_MOUSEDOWN, WM_MOUSEMOVE, and WM_MOUSEUP messages are not passed 
		// on to the underlying NativeWindow.
		UserMouse = 0x00000400,
		// If the BackColor is set to a color whose alpha component is
		// less than 255 (i.e., BackColor.A &lt; 255), OnPaintBackground will simulate transparency
		// by asking its parent control to paint our background.  This is not true transparency --
		// if there is another control between us and our parent, we will not show the control in the middle.
		SupportsTransparentBackColor = 0x00000800,
		// If set, windows forms calls OnDoubleClick and raises the DoubleClick event when the control is double clicked.
		// Regardless of whether it is set, the control may call OnDoubleClick directly.
		// This style is ignored if StandardClick is not set.
		StandardDoubleClick = 0x00001000,
		// If true, WM_ERASEBKGND is ignored, and both OnPaintBackground and OnPaint are called directly from
		// WM_PAINT.  This generally reduces flicker, but can cause problems if other controls
		// send WM_ERASEBKGND messages to us.  (This is sometimes done to achieve a pseudo-transparent effect similar to
		// ControlStyles.SupportsTransparentBackColor; for instance, ToolBar with flat appearance does this).
		// This style only makes sense if UserPaint is true.
		AllPaintingInWmPaint = 0x00002000,
		// If true, the control keeps a copy of the text rather than going to the hWnd for the
		// text every time. This improves performance but makes it difficult to keep the control
		// and hWnd's text synchronized. 
		// This style defaults to false.
		CacheText = 0x00004000,
		// If true, the OnNotifyMessage method will be called for every message sent to 
		// the control's WndProc. 
		// This style defaults to false.
		EnableNotifyMessage = 0x00008000,
		// If set, control painting is double buffered (OBSOLETE, Use OptimizedDoubleBuffer).
		DoubleBuffer = 0x00010000,
		// If set, all control painting will be double buffered.
		OptimizedDoubleBuffer = 0x00020000,
		// If this style is set, and there is a value in the control's Text property, that value will be
		// used to determine the control's default Active Accessibility name and shortcut key. Otherwise,
		// the text of the preceding Label control will be used instead.
		UseTextForAccessibility = 0x00040000,
	};

	//
	// Allow bitwise operations on this enumeration
	//
	constexpr ControlStyles operator|(ControlStyles Lhs, ControlStyles Rhs) 
	{
		return static_cast<ControlStyles>(static_cast<std::underlying_type<ControlStyles>::type>(Lhs) | static_cast<std::underlying_type<ControlStyles>::type>(Rhs)); 
	};
}