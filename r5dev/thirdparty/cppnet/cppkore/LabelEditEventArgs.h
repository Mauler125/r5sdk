#pragma once

#include <cstdint>
#include "StringBase.h"

namespace Forms
{
	// Provides data for the LabelEdit event.
	class LabelEditEventArgs
	{
	public:
		LabelEditEventArgs(int32_t Item);
		LabelEditEventArgs(int32_t Item, const string& Label);
		~LabelEditEventArgs() = default;

		// The new text for the item.
		string Label;
		// The index of the item being edited.
		int32_t Item;
		// Whether or not to cancel the changes.
		bool CancelEdit;
	};
}