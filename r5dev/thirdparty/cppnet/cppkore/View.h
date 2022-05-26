#pragma once

#include <cstdint>
#include <CommCtrl.h>

namespace Forms
{
	// Specifies how list items are displayed in a ListView control.
	enum class View
	{
		// Each item appears as a full-sized icon with a label below it.
		LargeIcon = LVS_ICON,
		// Each item appears on a separate line with further information
		// about each item in columns.
		Details = LVS_REPORT,
		// Each item appears as a small icon with a label to its right.
		SmallIcon = LVS_SMALLICON,
		// Each item appears as a small icon with a label to its right.
		List = LVS_LIST,
		// Tile view.
		Tile = LV_VIEW_TILE
	};
}