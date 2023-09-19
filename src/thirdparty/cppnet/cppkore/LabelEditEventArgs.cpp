#include "stdafx.h"
#include "LabelEditEventArgs.h"

namespace Forms
{
	LabelEditEventArgs::LabelEditEventArgs(int32_t Item)
		: Item(Item), Label(""), CancelEdit(false)
	{
	}

	LabelEditEventArgs::LabelEditEventArgs(int32_t Item, const String& Label)
		: Item(Item), Label(Label), CancelEdit(false)
	{
	}
}
