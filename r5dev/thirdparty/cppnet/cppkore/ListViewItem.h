#pragma once

#include <cstdint>
#include <memory>
#include <CommCtrl.h>
#include "StringBase.h"
#include "DrawingBase.h"

namespace Forms
{
	// The style for this specific sub item
	struct ListViewItemStyle
	{
		// The color behind the text of the item.
		Drawing::Color BackColor;
		// The color of the text for the item.
		Drawing::Color ForeColor;

		ListViewItemStyle()
			: BackColor(CLR_NONE), ForeColor(0, 0, 0)	// These match the recommended ListView colors
		{
		}
	};

	// An item used to represent an item in a ListView.
	class ListViewItem
	{
	public:
		ListViewItem();
		ListViewItem(std::initializer_list<string> SubItems);
		ListViewItem(std::initializer_list<string> SubItems, std::initializer_list<ListViewItemStyle> SubItemStyles);
		ListViewItem(const ListViewItem& Rhs);
		~ListViewItem() = default;

		// Assignment operator
		ListViewItem& operator=(const ListViewItem& Rhs);

		// Gets the text associated with this list item.
		const string& Text() const;
		// Gets the style associated with this list Item.
		const ListViewItemStyle& Style() const;

		// Gets the sub item text.
		const string& SubItem(uint32_t Index) const;
		// Gets the sub item style.
		const ListViewItemStyle& SubItemStyle(uint32_t Index) const;

		// Gets the count of sub items.
		uint32_t SubItemCount() const;

		// Gets or sets the index assigned to this list view item.
		int32_t Index;

	private:
		// Internal cached properties
		std::unique_ptr<string[]> _SubItems;
		std::unique_ptr<ListViewItemStyle[]> _SubItemStyles;
		uint32_t _SubItemCount;
	};
}