#pragma once

#include <cstdint>
#include "StringBase.h"
#include "ListViewItem.h"

namespace Forms
{
	// Provides data for the RetrieveVirtualItem event.
	class RetrieveVirtualItemEventArgs
	{
	public:
		RetrieveVirtualItemEventArgs(int32_t Index, int32_t SubIndex);
		~RetrieveVirtualItemEventArgs() = default;

		// The index of the item to get information on.
		const int32_t ItemIndex;
		// The sub item index.
		const int32_t SubItemIndex;
		// The text of the item.
		string Text;
		// The style of the item.
		ListViewItemStyle Style;
	};
}