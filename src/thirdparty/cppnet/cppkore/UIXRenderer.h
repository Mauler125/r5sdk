#pragma once

#include <cstdint>
#include "Control.h"
#include "ContentAlignment.h"
#include "PaintEventArgs.h"
#include "BufferedGraphics.h"
#include "DrawToolTipEventArgs.h"
#include "DrawListViewColumnHeaderEventArgs.h"
#include "DrawListViewItemEventArgs.h"
#include "DrawListViewSubItemEventArgs.h"

namespace UIX
{
	// The current control state
	enum class UIXRenderState
	{
		// The default state, enabled control
		Default,
		// The disabled control state
		Disabled,
		// The mouse over control state
		MouseOver,
		// The mouse down control state
		MouseDown,
		// The control is selected
		Selected,
	};

	// The render color types
	enum class UIXRenderColor
	{
		TextDefault,
		BackgroundDefault,
		BackgroundLight,
	};

	// A class that handles rendering UIX controls via a custom theme
	class UIXRenderer
	{
	public:
		UIXRenderer() = default;
		virtual ~UIXRenderer() = default;

		//
		// These must be implemented by default
		//

		virtual void RenderControlBorder(const std::unique_ptr<Forms::PaintEventArgs>& EventArgs, Forms::Control* Ctrl, UIXRenderState State) const = 0;
		virtual void RenderControlBackground(const std::unique_ptr<Forms::PaintEventArgs>& EventArgs, Forms::Control* Ctrl, UIXRenderState State) const = 0;
		virtual void RenderControlButtonBackground(const std::unique_ptr<Forms::PaintEventArgs>& EventArgs, Forms::Control* Ctrl, UIXRenderState State) const = 0;
		virtual void RenderControlText(const std::unique_ptr<Forms::PaintEventArgs>& EventArgs, Forms::Control* Ctrl, UIXRenderState State, Drawing::Rectangle LayoutRect, Drawing::ContentAlignment Alignment) const = 0;
		virtual void RenderControlProgressFill(const std::unique_ptr<Forms::PaintEventArgs>& EventArgs, Forms::Control* Ctrl, UIXRenderState State, uint32_t Progress) const = 0;
		virtual void RenderControlGlyph(const std::unique_ptr<Forms::PaintEventArgs>& EventArgs, Forms::Control* Ctrl, UIXRenderState State) const = 0;
		virtual void RenderControlCheckBoxBorder(const std::unique_ptr<Forms::PaintEventArgs>& EventArgs, Forms::Control* Ctrl, UIXRenderState State) const = 0;
		virtual void RenderControlCheckBoxCheck(const std::unique_ptr<Forms::PaintEventArgs>& EventArgs, Forms::Control* Ctrl, UIXRenderState State) const = 0;
		virtual void RenderControlRadioBorder(const std::unique_ptr<Forms::PaintEventArgs>& EventArgs, Forms::Control* Ctrl, UIXRenderState State) const = 0;
		virtual void RenderControlRadioCheck(const std::unique_ptr<Forms::PaintEventArgs>& EventArgs, Forms::Control* Ctrl, UIXRenderState State) const = 0;
		virtual void RenderControlGroupBox(const std::unique_ptr<Forms::PaintEventArgs>& EventArgs, Forms::Control* Ctrl, UIXRenderState State) const = 0;

		//
		// These are for ListView controls
		//

		virtual void RenderControlListHeader(const std::unique_ptr<Drawing::BufferedGraphics>& EventArgs) const = 0;
		virtual void RenderControlListColumnHeader(const std::unique_ptr<Forms::DrawListViewColumnHeaderEventArgs>& EventArgs, Forms::Control* Ctrl) const = 0;
		virtual void RenderControlListItem(const std::unique_ptr<Forms::DrawListViewItemEventArgs>& EventArgs, Forms::Control* Ctrl, Drawing::Rectangle SubItemBounds) const = 0;
		virtual void RenderControlListSubItem(const std::unique_ptr<Forms::DrawListViewSubItemEventArgs>& EventArgs, Forms::Control* Ctrl) const = 0;

		//
		// Tooltip renderer
		//

		virtual void RenderControlToolTip(const std::unique_ptr<Forms::DrawToolTipEventArgs>& EventArgs, Forms::Control* Ctrl) const = 0;

		//
		// These handle external colors
		//

		virtual Drawing::Color GetRenderColor(UIXRenderColor Color) const = 0;
	};
}