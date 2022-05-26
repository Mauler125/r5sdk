#pragma once

#include <cstdint>

namespace Forms
{
	// The event class that is created when the selection state of a ListViewItem is changed.
	class ListViewVirtualItemsSelectionRangeChangedEventArgs
	{
	public:
		ListViewVirtualItemsSelectionRangeChangedEventArgs(int32_t Start, int32_t End, bool Selected);
		~ListViewVirtualItemsSelectionRangeChangedEventArgs() = default;

		// The start of the new selection.
		const int32_t StartIndex;
		// The end of the new selection.
		const int32_t EndIndex;
		// Whether or not the range is selected or deselected.
		const bool IsSelected;
	};
}