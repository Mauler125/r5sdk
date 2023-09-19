#include "stdafx.h"
#include "UIXListView.h"
#include "UIXTheme.h"

namespace UIX
{
	UIX::UIXListView::UIXListView()
		: ListView::ListView()
	{
		this->SetOwnerDraw(true);
		this->SetDoubleBuffered(true);
		this->SetBorderStyle(Forms::BorderStyle::None);
		this->SetBackColor(UIXTheme::GetRenderer()->GetRenderColor(UIX::UIXRenderColor::BackgroundLight));
	}

	void UIXListView::OnHandleCreated()
	{
		// Initialize normally first
		ListView::OnHandleCreated();

		// Subclass the header
		this->_Header = std::make_unique<UIXListViewHeader>((HWND)SendMessageA(this->_Handle, LVM_GETHEADER, NULL, NULL), this);
	}

	void UIXListView::OnDrawItem(const std::unique_ptr<DrawListViewItemEventArgs>& EventArgs)
	{
		// We should do nothing here...
	}

	void UIXListView::OnDrawSubItem(const std::unique_ptr<DrawListViewSubItemEventArgs>& EventArgs)
	{
		UIXTheme::GetRenderer()->RenderControlListSubItem(EventArgs, this);
	}

	void UIXListView::OnDrawColumnHeader(const std::unique_ptr<DrawListViewColumnHeaderEventArgs>& EventArgs)
	{
		UIXTheme::GetRenderer()->RenderControlListColumnHeader(EventArgs, this);
	}
}
