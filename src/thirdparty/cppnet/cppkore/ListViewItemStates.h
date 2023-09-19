#pragma once

#include <cstdint>
#include <CommCtrl.h>

namespace Forms
{
	// Gives state information about a ListView item/sub-item. Used with owner draw.
	enum class ListViewItemStates
	{
		Checked = CDIS_CHECKED,
		Default = CDIS_DEFAULT,
		Focused = CDIS_FOCUS,
		Grayed = CDIS_GRAYED,
		Hot = CDIS_HOT,
		Indeterminate = CDIS_INDETERMINATE,
		Marked = CDIS_MARKED,
		Selected = CDIS_SELECTED,
		ShowKeyboardCues = CDIS_SHOWKEYBOARDCUES
	};
}