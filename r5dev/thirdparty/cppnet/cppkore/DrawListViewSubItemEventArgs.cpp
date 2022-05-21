#include "stdafx.h"
#include "DrawListViewSubItemEventArgs.h"

namespace Forms
{
	DrawListViewSubItemEventArgs::DrawListViewSubItemEventArgs(HDC Dc, const string& Text, const ListViewItemStyle Style, Drawing::Rectangle Bounds, int32_t ItemIndex, int32_t SubItemIndex, ListViewItemStates State)
		: Text(Text), Style(Style), Bounds(Bounds), ItemIndex(ItemIndex), SubItemIndex(SubItemIndex), DrawDefault(false), Graphics(std::make_unique<Drawing::Graphics>(Dc)), State(State)
	{
	}
}
