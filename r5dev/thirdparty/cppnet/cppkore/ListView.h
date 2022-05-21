#pragma once

#include <cstdint>
#include "View.h"
#include "ListBase.h"
#include "Control.h"
#include "SortOrder.h"
#include "BorderStyle.h"
#include "ColumnHeader.h"
#include "ListViewFlags.h"
#include "ListViewItem.h"
#include "ItemActivation.h"
#include "ListViewAlignment.h"
#include "ColumnHeaderStyle.h"
#include "LabelEditEventArgs.h"
#include "ListViewItemStates.h"
#include "ColumnClickEventArgs.h"
#include "DrawListViewItemEventArgs.h"
#include "CacheVirtualItemsEventArgs.h"
#include "RetrieveVirtualItemEventArgs.h"
#include "DrawListViewSubItemEventArgs.h"
#include "DrawListViewColumnHeaderEventArgs.h"
#include "ListViewVirtualItemsSelectionRangeChangedEventArgs.h"

namespace Forms
{
	// Displays a list of items in one of four
	// views. Each item displays a caption and optionally an image.
	class ListView : public Control
	{
	public:
		ListView();
		virtual ~ListView() = default;

		// Gets the style that specifies what kind of user action is required to activate an item.
		ItemActivation Activation();
		// Sets the style that specifies what kind of user action is required to activate an item.
		void SetActivation(ItemActivation Value);

		// The alignment style specifies which side of the window items are aligned to by default.
		ListViewAlignment Alignment();
		// The alignment style specifies which side of the window items are aligned to by default.
		void SetAlignment(ListViewAlignment Value);

		// Specifies whether the user can drag column headers.
		bool AllowColumnReorder();
		// Specifies whether the user can drag column headers.
		void SetAllowColumnReorder(bool Value);

		// If AutoArrange is true items are automatically arranged according to
		// the alignment property.
		bool AutoArrange();
		// If AutoArrange is true items are automatically arranged according to
		// the alignment property.
		void SetAutoArrange(bool Value);

		// Describes the border style of the window.
		BorderStyle GetBorderStyle();
		// Describes the border style of the window.
		void SetBorderStyle(BorderStyle Value);

		// The columns contained in the ListView.
		struct ColumnHeaderCollection
		{
			ColumnHeaderCollection(ListView* Owner);
			~ColumnHeaderCollection() = default;

			// Adds a column header to the ListView.
			void Add(const ColumnHeader& Value);
			// Inserts a column header to the ListView.
			void Insert(int32_t Index, const ColumnHeader& Value);
			// Removes all column headers from the ListView.
			void Clear();
			// Gets the index of a column header.
			int32_t IndexOf(const ColumnHeader& Value);
			// Gets the count of column headers in the ListView.
			uint32_t Count();

			// Iterater classes
			ColumnHeader* begin() const;
			ColumnHeader* end() const;

			// Array index operator
			ColumnHeader& operator[](uint32_t Index);

		protected:
			// Internal references
			ListView* _Owner;
			List<ColumnHeader> _Columns;
		} Columns;

		// Retreives the item which currently has the user focus.
		int32_t FocusedItem();
		// Sets the item which currently has the user focus.
		void SetFocusedItem(int32_t Value);

		// Specifies whether a click on an item will select the entire row.
		bool FullRowSelect();
		// Specifies whether a click on an item will select the entire row.
		void SetFullRowSelect(bool Value);

		// If true, draws grid lines between items and subItems.
		bool GridLines();
		// If true, draws grid lines between items and subItems.
		void SetGridLines(bool Value);

		// Column headers can either be invisible, clickable, or non-clickable.
		ColumnHeaderStyle HeaderStyle();
		// Column headers can either be invisible, clickable, or non-clickable.
		void SetHeaderStyle(ColumnHeaderStyle Value);

		// If false, selected items will still be highlighted.
		bool HideSelection();
		// If false, selected items will still be highlighted.
		void SetHideSelection(bool Value);

		// Enables or disables hot tracking.
		bool HotTracking();
		// Enables or disables hot tracking.
		void SetHotTracking(bool Value);

		// Determines whether items can be selected by hovering over them with the mouse.
		bool HoverSelection();
		// Determines whether items can be selected by hovering over them with the mouse.
		void SetHoverSelection(bool Value);

		// The items contained in the ListView if not virtual.
		struct ListViewItemCollection
		{
			ListViewItemCollection(ListView* Owner);
			~ListViewItemCollection() = default;

			// Adds an item to the list view.
			void Add(const ListViewItem& Value);
			// Inserts an item to the list view.
			void Insert(int32_t Index, const ListViewItem& Value);
			// Removes an item at the given index.
			void RemoveAt(int32_t Index);
			// Clears the items in the list view.
			void Clear();
			// Gets the count of items in the list view.
			uint32_t Count();
			
			// Array index operator
			ListViewItem& operator[](uint32_t Index);

			// Iterater classes
			ListViewItem* begin() const;
			ListViewItem* end() const;

			// Gets a list of items from the collection
			const List<ListViewItem>& ToList();

		protected:
			ListView* _Owner;
			List<ListViewItem> _Items;
		} Items;

		// Tells whether the EditLabels style is currently set.
		bool LabelEdit();
		// Tells whether the EditLabels style is currently set.
		void SetLabelEdit(bool Value);

		// Tells whether the LabelWrap style is currently set.
		bool LabelWrap();
		// Tells whether the LabelWrap style is currently set.
		void SetLabelWrap(bool Value);

		// Tells whether the MultiSelect style is currently set.
		bool MultiSelect();
		// Tells whether the MultiSelect style is currently set.
		void SetMultiSelect(bool Value);

		// Indicates whether the list view items (and sub-items in the Details view) will be
		// drawn by the system or the user.
		bool OwnerDraw();
		// Indicates whether the list view items (and sub-items in the Details view) will be
		// drawn by the system or the user.
		void SetOwnerDraw(bool Value);

		// Tells whether the ScrollBars are visible or not.
		bool Scrollable();
		// Tells whether the ScrollBars are visible or not.
		void SetScrollable(bool Value);

		// The indices of the currently selected list items.
		List<uint32_t> SelectedIndices();

		// Tells whether or not groups are visible.
		bool ShowGroups();
		// Tells whether or not groups are visible.
		void SetShowGroups(bool Value);

		// Gets the sorting order for items.
		SortOrder Sorting();
		// Sets the sorting order for items.
		void SetSorting(SortOrder Value);

		// Gets the top item index.
		int32_t TopItem();
		// Sets the top item by it's index.
		void SetTopItem(int32_t Value);

		// Gets the total amount of visible items of the ListView.
		int32_t CountPerPage();

		// Gets or sets the view mode of the ListView.
		View GetView();
		// Gets or sets the view mode of the ListView.
		void SetView(View Value);

		// Gets the number of items in the virtual list.
		int32_t VirtualListSize();
		// Sets the number of items in the virtual list.
		void SetVirtualListSize(int32_t Value);

		// Gets whether or not the ListView is virtual.
		bool VirtualMode();
		// Sets whether or not the ListView is virtual.
		void SetVirtualMode(bool Value);

		// Used change column info.
		void SetColumnInfo(int32_t Mask, ColumnHeader& Header);
		// Used to change column widths.
		void SetColumnWidth(int32_t Index, int32_t Width);

		// We must define control event bases here
		virtual void OnHandleCreated();
		virtual void OnBackColorChanged();
		virtual void OnForeColorChanged();
		virtual void OnFontChanged();
		virtual void OnItemActivate();
		virtual void OnColumnClick(const std::unique_ptr<ColumnClickEventArgs>& EventArgs);
		virtual void OnBeforeLabelEdit(const std::unique_ptr<LabelEditEventArgs>& EventArgs);
		virtual void OnAfterLabelEdit(const std::unique_ptr<LabelEditEventArgs>& EventArgs);
		virtual void OnDrawItem(const std::unique_ptr<DrawListViewItemEventArgs>& EventArgs);
		virtual void OnDrawSubItem(const std::unique_ptr<DrawListViewSubItemEventArgs>& EventArgs);
		virtual void OnDrawColumnHeader(const std::unique_ptr<DrawListViewColumnHeaderEventArgs>& EventArgs);
		virtual void OnCacheVirtualItems(const std::unique_ptr<CacheVirtualItemsEventArgs>& EventArgs);
		virtual void OnRetrieveVirtualItem(const std::unique_ptr<RetrieveVirtualItemEventArgs>& EventArgs);
		virtual void OnVirtualItemsSelectionRangeChanged(const std::unique_ptr<ListViewVirtualItemsSelectionRangeChangedEventArgs>& EventArgs);

		// We must define event handlers here
		EventBase<void(*)(Control*)> ItemActivate;
		EventBase<void(*)(const std::unique_ptr<ColumnClickEventArgs>&, Control*)> ColumnClick;
		EventBase<void(*)(const std::unique_ptr<LabelEditEventArgs>&, Control*)> BeforeLabelEdit;
		EventBase<void(*)(const std::unique_ptr<LabelEditEventArgs>&, Control*)> AfterLabelEdit;
		EventBase<void(*)(const std::unique_ptr<DrawListViewItemEventArgs>&, Control*)> DrawItem;
		EventBase<void(*)(const std::unique_ptr<DrawListViewSubItemEventArgs>&, Control*)> DrawSubItem;
		EventBase<void(*)(const std::unique_ptr<DrawListViewColumnHeaderEventArgs>&, Control*)> DrawColumnHeader;
		EventBase<void(*)(const std::unique_ptr<CacheVirtualItemsEventArgs>&, Control*)> CacheVirtualItems;
		EventBase<void(*)(const std::unique_ptr<RetrieveVirtualItemEventArgs>&, Control*)> RetrieveVirtualItem;
		EventBase<void(*)(const std::unique_ptr<ListViewVirtualItemsSelectionRangeChangedEventArgs>&, Control*)> VirtualItemsSelectionRangeChanged;

		// Override WndProc for specific combo box messages.
		virtual void WndProc(Message& Msg);

		// Creates a new instance of the specified control with the given parent.
		virtual void CreateControl(Control* Parent = nullptr);

	protected:
		// Get custom control creation parameters for this instance.
		virtual CreateParams GetCreateParams();
		// Internal routine to scale a control by factor
		virtual void ScaleControl(Drawing::SizeF Factor, BoundsSpecified Specified);

		// Internal cached flags
		ListViewFlags _Flags;

		// Gets list view specific flags
		bool GetFlag(ListViewFlags Flag);
		// Sets list view specific flags
		void SetFlag(ListViewFlags Flags, bool Value);

		// Internal routine to add a column
		void NativeAddColumn(ColumnHeader& Header);
		// Internal routine to insert a column
		void NativeInsertColumn(const int32_t Index, ColumnHeader& Header);
		// Internal routine to clear the columns
		void NativeClearColumns();

		// Internal routine to add an item at the given index
		void NativeInsertItem(int32_t Index, const ListViewItem& Item);
		// Internal routine to insert items at the given index
		void NativeInsertItems(int32_t Index, int32_t Count, const List<ListViewItem>& Items);
		// Internal routine to remove an item
		void NativeRemoveItem(int32_t Index);
		// Internal routine to clear all items
		void NativeClearItems();

		// Internal routine to invalidate headers
		void InvalidateColumnHeaders();

		// Internal routine to get an item position
		Drawing::Point GetItemPosition(int32_t Index);
		// Internal routine to get an item rectangle
		Drawing::Rectangle GetItemRectOrEmpty(int32_t Index);
		// Internal routine to get the sub item rectangle
		Drawing::Rectangle GetSubItemRect(int32_t Index, int32_t SubItem);
		// Internal routine to get an item state
		int32_t GetItemState(int32_t Index);

		// Internal routine to get a sub item
		std::pair<string, ListViewItemStyle> GetSubItem(int32_t Index, int32_t SubItem);

		// Internal routine to set column indices
		void SetDisplayIndices(const std::unique_ptr<int32_t[]>& Indices, int32_t Count);

		// Internal routine to update styles
		void UpdateExtendedStyles();

	private:
		// Internal cached flags
		ItemActivation _ItemActivation;
		ListViewAlignment _ListViewAlignment;
		BorderStyle _BorderStyle;
		ColumnHeaderStyle _ColumnHeaderStyle;
		SortOrder _SortOrder;
		View _ViewStyle;
		MouseButtons _DownButton;

		// Virtual mode size / column index
		int32_t _VirtualListSize;
		int32_t _ColumnIndex;

		// Internal routine to get index of an item
		int32_t GetIndexOfClickedItem(LVHITTESTINFO& LvHI);

		// We must define each window message handler here...
		void WmMouseDown(Message& Msg, MouseButtons Button, uint32_t Clicks);
		void WmReflectNotify(Message& Msg);
		bool WmNotify(Message& Msg);
		void NmCustomDraw(Message& Msg);
	};
}