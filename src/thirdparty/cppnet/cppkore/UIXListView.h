#pragma once

#include <memory>
#include "ListView.h"
#include "UIXListViewHeader.h"

using namespace Forms;

namespace UIX
{
	// Represents a UIX themed listview control.
	class UIXListView : public ListView
	{
	public:
		UIXListView();

		// We must define base events here
		virtual void OnHandleCreated();

		// Implement the virtual calls to fix the layout
		virtual void OnDrawItem(const std::unique_ptr<DrawListViewItemEventArgs>& EventArgs);
		virtual void OnDrawSubItem(const std::unique_ptr<DrawListViewSubItemEventArgs>& EventArgs);
		virtual void OnDrawColumnHeader(const std::unique_ptr<DrawListViewColumnHeaderEventArgs>& EventArgs);

	private:
		// An internal instance for the header
		std::unique_ptr<UIXListViewHeader> _Header;
	};
}