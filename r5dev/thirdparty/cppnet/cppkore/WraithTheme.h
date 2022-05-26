#pragma once

#include <memory>
#include "DrawingBase.h"
#include "ContentAlignment.h"
#include "UIXRenderer.h"

namespace Themes
{
	class WraithTheme : public UIX::UIXRenderer
	{
	public:
		WraithTheme();
		virtual ~WraithTheme();

	private:
		// Inherited via UIXRenderer
		virtual void RenderControlBorder(const std::unique_ptr<Forms::PaintEventArgs>& EventArgs, Forms::Control* Ctrl, UIX::UIXRenderState State) const override;
		virtual void RenderControlBackground(const std::unique_ptr<Forms::PaintEventArgs>& EventArgs, Forms::Control* Ctrl, UIX::UIXRenderState State) const override;
		virtual void RenderControlButtonBackground(const std::unique_ptr<Forms::PaintEventArgs>& EventArgs, Forms::Control* Ctrl, UIX::UIXRenderState State) const override;
		virtual void RenderControlText(const std::unique_ptr<Forms::PaintEventArgs>& EventArgs, Forms::Control* Ctrl, UIX::UIXRenderState State, Drawing::Rectangle LayoutRect, Drawing::ContentAlignment Alignment) const override;
		virtual void RenderControlProgressFill(const std::unique_ptr<Forms::PaintEventArgs>& EventArgs, Forms::Control* Ctrl, UIX::UIXRenderState State, uint32_t Progress) const override;
		virtual void RenderControlGlyph(const std::unique_ptr<Forms::PaintEventArgs>& EventArgs, Forms::Control* Ctrl, UIX::UIXRenderState State) const override;
		virtual void RenderControlCheckBoxBorder(const std::unique_ptr<Forms::PaintEventArgs>& EventArgs, Forms::Control* Ctrl, UIX::UIXRenderState State) const override;
		virtual void RenderControlCheckBoxCheck(const std::unique_ptr<Forms::PaintEventArgs>& EventArgs, Forms::Control* Ctrl, UIX::UIXRenderState State) const override;
		virtual void RenderControlRadioBorder(const std::unique_ptr<Forms::PaintEventArgs>& EventArgs, Forms::Control* Ctrl, UIX::UIXRenderState State) const override;
		virtual void RenderControlRadioCheck(const std::unique_ptr<Forms::PaintEventArgs>& EventArgs, Forms::Control* Ctrl, UIX::UIXRenderState State) const override;
		virtual void RenderControlGroupBox(const std::unique_ptr<Forms::PaintEventArgs>& EventArgs, Forms::Control* Ctrl, UIX::UIXRenderState State) const override;

		// Inherited via UIXRenderer
		virtual void RenderControlListHeader(const std::unique_ptr<Drawing::BufferedGraphics>& EventArgs) const override;
		virtual void RenderControlListColumnHeader(const std::unique_ptr<Forms::DrawListViewColumnHeaderEventArgs>& EventArgs, Forms::Control* Ctrl) const override;
		virtual void RenderControlListItem(const std::unique_ptr<Forms::DrawListViewItemEventArgs>& EventArgs, Forms::Control* Ctrl, Drawing::Rectangle SubItemBounds) const override;
		virtual void RenderControlListSubItem(const std::unique_ptr<Forms::DrawListViewSubItemEventArgs>& EventArgs, Forms::Control* Ctrl) const override;

		// Inherited via UIXRenderer
		virtual Drawing::Color GetRenderColor(UIX::UIXRenderColor Color) const override;
	};
}