#include "stdafx.h"
#include "DrawListViewColumnHeaderEventArgs.h"

namespace Forms
{
	DrawListViewColumnHeaderEventArgs::DrawListViewColumnHeaderEventArgs(HDC Dc, const ColumnHeader* Header, int32_t ColumnIndex, Drawing::Rectangle Bounds, ListViewItemStates State)
		: Header(Header), ColumnIndex(ColumnIndex), Bounds(Bounds), State(State), DrawDefault(false), Graphics(std::make_unique<Drawing::Graphics>(Dc))
	{
	}
}
