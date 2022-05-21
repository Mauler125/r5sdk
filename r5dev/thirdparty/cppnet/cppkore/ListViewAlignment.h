#pragma once

#include <cstdint>
#include <CommCtrl.h>

namespace Forms
{
	// Specifies how items align in the ListView
	enum class ListViewAlignment
	{
		// When the user moves an
		// item, it remains where it is dropped.
		Default = LVA_DEFAULT,
		// Items are aligned to the top of the ListView control.
		Top = LVA_ALIGNTOP,
		// Items are aligned to the left of the ListView control.
		Left = LVA_ALIGNLEFT,
		// Items are aligned to an invisible grid in the control.
		SnapToGrid = LVA_SNAPTOGRID
	};
}