#pragma once

#include <memory>
#include <cstdint>
#include "StringBase.h"
#include "DrawingBase.h"
#include "ListViewItem.h"
#include "ListViewItemStates.h"

namespace Forms
{
	// This class contains the information a user needs to paint ListView items.
	class DrawListViewItemEventArgs
	{
	public:
		DrawListViewItemEventArgs(HDC Dc, const string& Text, const ListViewItemStyle Style, Drawing::Rectangle Bounds, int32_t ItemIndex, ListViewItemStates State);
		~DrawListViewItemEventArgs() = default;

		// Gets the text of the item to draw.
		const string Text;
		// Gets the style of the item to draw.
		const ListViewItemStyle Style;
		// Gets the state of the item to draw.
		const ListViewItemStates State;
		// Gets the bounds of the item to draw.
		const Drawing::Rectangle Bounds;
		// The index of the item to draw.
		const int32_t ItemIndex;

		// Whether or not the system draws the item.
		bool DrawDefault;

		// The graphics instance used to paint this item.
		std::unique_ptr<Drawing::Graphics> Graphics;
	};
}