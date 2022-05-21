#include "stdafx.h"
#include "DrawListViewItemEventArgs.h"

namespace Forms
{
	DrawListViewItemEventArgs::DrawListViewItemEventArgs(HDC Dc, const string& Text, const ListViewItemStyle Style, Drawing::Rectangle Bounds, int32_t ItemIndex, ListViewItemStates State)
		: Text(Text), Style(Style), Bounds(Bounds), ItemIndex(ItemIndex), DrawDefault(false), Graphics(std::make_unique<Drawing::Graphics>(Dc)), State(State)
	{
	}
}
