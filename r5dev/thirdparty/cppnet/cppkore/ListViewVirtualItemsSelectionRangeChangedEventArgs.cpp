#include "stdafx.h"
#include "ListViewVirtualItemsSelectionRangeChangedEventArgs.h"

namespace Forms
{
	ListViewVirtualItemsSelectionRangeChangedEventArgs::ListViewVirtualItemsSelectionRangeChangedEventArgs(int32_t Start, int32_t End, bool Selected)
		: StartIndex(Start), EndIndex(End), IsSelected(Selected)
	{
	}
}
