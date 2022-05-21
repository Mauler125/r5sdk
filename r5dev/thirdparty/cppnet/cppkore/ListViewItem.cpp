#include "stdafx.h"
#include "ListViewItem.h"

namespace Forms
{
	ListViewItem::ListViewItem()
		: _SubItems(nullptr), _SubItemStyles(nullptr), Index(-1), _SubItemCount(0)
	{
	}

	ListViewItem::ListViewItem(std::initializer_list<string> SubItems)
		: _SubItems(std::make_unique<string[]>(SubItems.size())), _SubItemStyles(std::make_unique<ListViewItemStyle[]>(SubItems.size())), Index(-1), _SubItemCount((uint32_t)SubItems.size())
	{
		std::copy(SubItems.begin(), SubItems.end(), _SubItems.get());
	}

	ListViewItem::ListViewItem(std::initializer_list<string> SubItems, std::initializer_list<ListViewItemStyle> SubItemStyles)
		: _SubItems(std::make_unique<string[]>(SubItems.size())), _SubItemStyles(std::make_unique<ListViewItemStyle[]>(SubItemStyles.size())), Index(-1), _SubItemCount((uint32_t)SubItems.size())
	{
		std::copy(SubItems.begin(), SubItems.end(), _SubItems.get());
		std::copy(SubItemStyles.begin(), SubItemStyles.end(), _SubItemStyles.get());
	}

	ListViewItem::ListViewItem(const ListViewItem& Rhs)
	{
		this->_SubItemCount = Rhs._SubItemCount;

		this->_SubItems = std::make_unique<string[]>(_SubItemCount);
		this->_SubItemStyles = std::make_unique<ListViewItemStyle[]>(_SubItemCount);

		std::copy(Rhs._SubItems.get(), Rhs._SubItems.get() + _SubItemCount, this->_SubItems.get());
		std::copy(Rhs._SubItemStyles.get(), Rhs._SubItemStyles.get() + _SubItemCount, this->_SubItemStyles.get());
	}

	ListViewItem& ListViewItem::operator=(const ListViewItem& Rhs)
	{
		this->_SubItemCount = Rhs._SubItemCount;

		this->_SubItems = std::make_unique<string[]>(_SubItemCount);
		this->_SubItemStyles = std::make_unique<ListViewItemStyle[]>(_SubItemCount);

		std::copy(Rhs._SubItems.get(), Rhs._SubItems.get() + _SubItemCount, this->_SubItems.get());
		std::copy(Rhs._SubItemStyles.get(), Rhs._SubItemStyles.get() + _SubItemCount, this->_SubItemStyles.get());

		return *this;
	}

	const string& ListViewItem::Text() const
	{
		return _SubItems[0];
	}

	const ListViewItemStyle& ListViewItem::Style() const
	{
		return _SubItemStyles[0];
	}

	const string& ListViewItem::SubItem(uint32_t Index) const
	{
		return _SubItems[Index];
	}

	const ListViewItemStyle& ListViewItem::SubItemStyle(uint32_t Index) const
	{
		return _SubItemStyles[Index];
	}

	uint32_t ListViewItem::SubItemCount() const
	{
		return _SubItemCount - 1;
	}
}
