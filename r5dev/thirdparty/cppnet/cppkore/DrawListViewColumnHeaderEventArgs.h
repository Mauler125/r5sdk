#pragma once

#include <cstdint>
#include <memory>
#include "DrawingBase.h"
#include "ColumnHeader.h"
#include "ListViewItemStates.h"

namespace Forms
{
	// This class contains the information a user needs to paint ListView headers.
	class DrawListViewColumnHeaderEventArgs
	{
	public:
		DrawListViewColumnHeaderEventArgs(HDC Dc, const ColumnHeader* Header, int32_t ColumnIndex, Drawing::Rectangle Bounds, ListViewItemStates State);
		~DrawListViewColumnHeaderEventArgs() = default;

		// The header object.
		const ColumnHeader* Header;
		// Gets the state of the header to draw.
		const ListViewItemStates State;
		// Gets the bounds of the header to draw.
		const Drawing::Rectangle Bounds;
		// The index of the header to draw.
		const int32_t ColumnIndex;

		// Whether or not the system draws the header.
		bool DrawDefault;

		// The graphics instance used to paint this header.
		std::unique_ptr<Drawing::Graphics> Graphics;
	};
}