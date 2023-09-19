#pragma once

#include <cstdint>
#include <type_traits>

namespace Forms
{
	// This enumeration represents the ListView flags...
	enum class ListViewFlags
	{
		FlagOwnerDraw = 0x00000001,
		FlagAllowColumnReorder = 0x00000002,
		FlagAutoArrange = 0x00000004,
		FlagCheckBoxes = 0x00000008,
		FlagFullRowSelect = 0x00000010,
		FlagGridLines = 0x00000020,
		FlagHideSelection = 0x00000040,
		FlagHotTracking = 0x00000080,
		FlagLabelEdit = 0x00000100,
		FlagLabelWrap = 0x00000200,
		FlagMultiSelect = 0x00000400,
		FlagScrollable = 0x00000800,
		FlagHoverSelection = 0x00001000,
		FlagMonclickHdr = 0x00002000,
		FlagInLabelEdit = 0x00004000,
		FlagShowItemToolTips = 0x00008000,
		FlagBackgroundImageTiled = 0x00010000,
		FlagColumnClicked = 0x00020000,
		FlagDoubleclickFired = 0x00040000,
		FlagMouseUpFired = 0x00080000,
		FlagExpectingMouseUp = 0x00100000,
		FlagComctlSupportsVisualStyles = 0x00200000,
		FlagComctlSupportsVisualStylesTested = 0x00400000,
		FlagShowGroups = 0x00800000,
		FlagHandleDestroyed = 0x01000000,
		FlagVirtualMode = 0x02000000,
		FlagHeaderControlTracking = 0x04000000,
		FlagItemCollectionChangedInMouseDown = 0x08000000,
		FlagFlipViewToLargeIconAndSmallIcon = 0x10000000,
		FlagHeaderDividerDblClick = 0x20000000,
		FlagColumnResizeCancelled = 0x40000000
	};

	//
	// Allow bitwise operations on this enumeration
	//
	constexpr ListViewFlags operator|(ListViewFlags Lhs, ListViewFlags Rhs)
	{
		return static_cast<ListViewFlags>(static_cast<std::underlying_type<ListViewFlags>::type>(Lhs) | static_cast<std::underlying_type<ListViewFlags>::type>(Rhs));
	};
}