#include "stdafx.h"
#include "ListView.h"

namespace Forms
{
	ListView::ListView()
		: Control(), Columns(this), Items(this), _ItemActivation(ItemActivation::Standard), _ListViewAlignment(ListViewAlignment::Top), _BorderStyle(BorderStyle::Fixed3D), _ColumnHeaderStyle(ColumnHeaderStyle::Clickable), _SortOrder(SortOrder::None), _ViewStyle(View::LargeIcon), _VirtualListSize(0), _ColumnIndex(0), _DownButton(MouseButtons::None), _Flags(ListViewFlags::FlagScrollable | ListViewFlags::FlagMultiSelect | ListViewFlags::FlagLabelWrap | ListViewFlags::FlagHideSelection | ListViewFlags::FlagAutoArrange | ListViewFlags::FlagShowGroups)
	{
		SetStyle(ControlStyles::UserPaint | ControlStyles::StandardClick | ControlStyles::UseTextForAccessibility, false);

		// Default back color is different...
		this->_BackColor = Drawing::GetSystemColor(Drawing::SystemColors::Window);

		// We are a ListView control.
		this->_RTTI = ControlTypes::ListView;
	}

	ItemActivation ListView::Activation()
	{
		return _ItemActivation;
	}

	void ListView::SetActivation(ItemActivation Value)
	{
		if (_ItemActivation != Value)
		{
			_ItemActivation = Value;
			UpdateExtendedStyles();
		}
	}

	ListViewAlignment ListView::Alignment()
	{
		return this->_ListViewAlignment;
	}

	void ListView::SetAlignment(ListViewAlignment Value)
	{
		if (_ListViewAlignment != Value)
		{
			_ListViewAlignment = Value;
			UpdateStyles();
		}
	}

	bool ListView::AllowColumnReorder()
	{
		return GetFlag(ListViewFlags::FlagAllowColumnReorder);
	}

	void ListView::SetAllowColumnReorder(bool Value)
	{
		if (AllowColumnReorder() != Value)
		{
			SetFlag(ListViewFlags::FlagAllowColumnReorder, Value);
			UpdateExtendedStyles();
		}
	}

	bool ListView::AutoArrange()
	{
		return GetFlag(ListViewFlags::FlagAutoArrange);
	}

	void ListView::SetAutoArrange(bool Value)
	{
		if (AutoArrange() != Value)
		{
			SetFlag(ListViewFlags::FlagAutoArrange, Value);
			UpdateStyles();
		}
	}

	BorderStyle ListView::GetBorderStyle()
	{
		return this->_BorderStyle;
	}

	void ListView::SetBorderStyle(BorderStyle Value)
	{
		if (_BorderStyle != Value)
		{
			_BorderStyle = Value;
			UpdateStyles();
		}
	}

	int32_t ListView::FocusedItem()
	{
		return (int32_t)SendMessageA(this->_Handle, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_FOCUSED);
	}

	void ListView::SetFocusedItem(int32_t Value)
	{
		if (GetState(ControlStates::StateCreated))
		{
			LVITEMA LvItem{};
			LvItem.mask = LVIF_STATE;
			LvItem.state = LVNI_FOCUSED;
			LvItem.stateMask = LVIS_FOCUSED;

			SendMessageA(this->_Handle, LVM_SETITEMSTATE, (WPARAM)Value, (LPARAM)&LvItem);
		}
	}

	bool ListView::FullRowSelect()
	{
		return GetFlag(ListViewFlags::FlagFullRowSelect);
	}

	void ListView::SetFullRowSelect(bool Value)
	{
		if (FullRowSelect() != Value)
		{
			SetFlag(ListViewFlags::FlagFullRowSelect, Value);
			UpdateExtendedStyles();
		}
	}

	bool ListView::GridLines()
	{
		return GetFlag(ListViewFlags::FlagGridLines);
	}

	void ListView::SetGridLines(bool Value)
	{
		if (GridLines() != Value)
		{
			SetFlag(ListViewFlags::FlagGridLines, Value);
			UpdateExtendedStyles();
		}
	}

	ColumnHeaderStyle ListView::HeaderStyle()
	{
		return this->_ColumnHeaderStyle;
	}

	void ListView::SetHeaderStyle(ColumnHeaderStyle Value)
	{
		if (_ColumnHeaderStyle != Value)
		{
			_ColumnHeaderStyle = Value;
			UpdateStyles();
		}
	}

	bool ListView::HideSelection()
	{
		return GetFlag(ListViewFlags::FlagHideSelection);
	}

	void ListView::SetHideSelection(bool Value)
	{
		if (HideSelection() != Value)
		{
			SetFlag(ListViewFlags::FlagHideSelection, Value);
			UpdateStyles();
		}
	}

	bool ListView::HotTracking()
	{
		return GetFlag(ListViewFlags::FlagHotTracking);
	}

	void ListView::SetHotTracking(bool Value)
	{
		if (HotTracking() != Value)
		{
			SetFlag(ListViewFlags::FlagHotTracking, Value);

			if (Value)
			{
				SetHoverSelection(true);
				SetActivation(ItemActivation::OneClick);
			}
		}
	}

	bool ListView::HoverSelection()
	{
		return GetFlag(ListViewFlags::FlagHoverSelection);
	}

	void ListView::SetHoverSelection(bool Value)
	{
		if (HoverSelection() != Value)
		{
			SetFlag(ListViewFlags::FlagHoverSelection, Value);
			UpdateExtendedStyles();
		}
	}

	bool ListView::LabelEdit()
	{
		return GetFlag(ListViewFlags::FlagLabelEdit);
	}

	void ListView::SetLabelEdit(bool Value)
	{
		if (LabelEdit() != Value)
		{
			SetFlag(ListViewFlags::FlagLabelEdit, Value);
			UpdateStyles();
		}
	}

	bool ListView::LabelWrap()
	{
		return GetFlag(ListViewFlags::FlagLabelWrap);
	}

	void ListView::SetLabelWrap(bool Value)
	{
		if (LabelWrap() != Value)
		{
			SetFlag(ListViewFlags::FlagLabelWrap, Value);
			UpdateStyles();
		}
	}

	bool ListView::MultiSelect()
	{
		return GetFlag(ListViewFlags::FlagMultiSelect);
	}

	void ListView::SetMultiSelect(bool Value)
	{
		if (MultiSelect() != Value)
		{
			SetFlag(ListViewFlags::FlagMultiSelect, Value);
			UpdateStyles();
		}
	}

	bool ListView::OwnerDraw()
	{
		return GetFlag(ListViewFlags::FlagOwnerDraw);
	}

	void ListView::SetOwnerDraw(bool Value)
	{
		if (OwnerDraw() != Value)
		{
			SetFlag(ListViewFlags::FlagOwnerDraw, Value);
			Invalidate(true);
		}
	}

	bool ListView::Scrollable()
	{
		return GetFlag(ListViewFlags::FlagScrollable);
	}

	void ListView::SetScrollable(bool Value)
	{
		if (Scrollable() != Value)
		{
			SetFlag(ListViewFlags::FlagScrollable, Value);
			UpdateStyles();
		}
	}

	List<uint32_t> ListView::SelectedIndices()
	{
		// Fetch count of selected items
		int32_t SelectedCount = (int32_t)(GetState(ControlStates::StateCreated) ? SendMessageA(this->_Handle, LVM_GETSELECTEDCOUNT, NULL, NULL) : -1);

		// Ignore false counts
		if (SelectedCount <= 0)
			return List<uint32_t>();

		// Generate a new list
		List<uint32_t> Result(SelectedCount, true);

		int32_t LastIndex = -1;
		for (int32_t i = 0; i < SelectedCount; i++)
		{
			LastIndex = (int32_t)SendMessageA(this->_Handle, LVM_GETNEXTITEM, (WPARAM)LastIndex, LVNI_SELECTED);
			Result[i] = (uint32_t)LastIndex;
		}

		return Result;
	}

	bool ListView::ShowGroups()
	{
		return GetFlag(ListViewFlags::FlagShowGroups);
	}

	void ListView::SetShowGroups(bool Value)
	{
		if (ShowGroups() != Value)
		{
			SetFlag(ListViewFlags::FlagShowGroups, Value);
		}
	}

	SortOrder ListView::Sorting()
	{
		return this->_SortOrder;
	}

	void ListView::SetSorting(SortOrder Value)
	{
		if (_SortOrder != Value)
		{
			_SortOrder = Value;
			UpdateStyles();
		}
	}

	int32_t ListView::TopItem()
	{
		return (int32_t)(GetState(ControlStates::StateCreated) ? SendMessageA(this->_Handle, LVM_GETTOPINDEX, NULL, NULL) : -1);
	}

	void ListView::SetTopItem(int32_t Value)
	{
		if (GetState(ControlStates::StateCreated))
		{
			if (Value <= 0)
				Value = 0;

			// Make sure it's visible first
			SendMessageA(this->_Handle, LVM_ENSUREVISIBLE, (WPARAM)Value, NULL);

			if (Scrollable())
			{
				// Scroll to top
				int32_t ScrollY = GetItemPosition(0).Y - GetItemPosition(Value).Y;
				SendMessageA(this->_Handle, LVM_SCROLL, 0, ScrollY);
			}
		}
	}

	int32_t ListView::CountPerPage()
	{
		return (int32_t)SendMessageA(this->_Handle, LVM_GETCOUNTPERPAGE, NULL, NULL);
	}

	View ListView::GetView()
	{
		return this->_ViewStyle;
	}

	void ListView::SetView(View Value)
	{
		if (_ViewStyle != Value)
		{
			_ViewStyle = Value;

			if (GetState(ControlStates::StateCreated))
			{
				SendMessageA(this->_Handle, LVM_SETVIEW, (WPARAM)_ViewStyle, NULL);
			}
			else
			{
				UpdateStyles();
			}
		}
	}

	int32_t ListView::VirtualListSize()
	{
		return this->_VirtualListSize;
	}

	void ListView::SetVirtualListSize(int32_t Value)
	{
		if (_VirtualListSize == Value)
			return;

		bool KeepTopItem = GetState(ControlStates::StateCreated) && this->VirtualMode() && this->_ViewStyle == View::Details;
		int32_t TopIndex = -1;

		if (KeepTopItem)
			TopIndex = (int32_t)SendMessageA(this->_Handle, LVM_GETTOPINDEX, NULL, NULL);

		_VirtualListSize = Value;

		if (GetState(ControlStates::StateCreated) && this->VirtualMode())
			SendMessageA(this->_Handle, LVM_SETITEMCOUNT, (WPARAM)_VirtualListSize, NULL);

		if (KeepTopItem)
		{
			TopIndex = min(TopIndex, this->_VirtualListSize - 1);

			if (TopIndex > 0)
				this->SetTopItem(TopIndex);
		}
	}

	bool ListView::VirtualMode()
	{
		return GetFlag(ListViewFlags::FlagVirtualMode);
	}

	void ListView::SetVirtualMode(bool Value)
	{
		if (VirtualMode() != Value)
		{
			SetFlag(ListViewFlags::FlagVirtualMode, Value);
			UpdateStyles();
		}
	}

	void ListView::OnHandleCreated()
	{
		// Call tbe base event first
		Control::OnHandleCreated();

		// Ensure we have a proper comctl version
		int32_t Version = (int32_t)SendMessageA(this->_Handle, CCM_GETVERSION, NULL, NULL);
		if (Version < 5)
			SendMessageA(this->_Handle, CCM_SETVERSION, (WPARAM)5, NULL);

		// Ensure extended styles are set
		UpdateExtendedStyles();

		// Update control colors
		SendMessageA(this->_Handle, LVM_SETBKCOLOR, NULL, (LPARAM)Drawing::ColorToWin32(this->BackColor()));
		SendMessageA(this->_Handle, LVM_SETTEXTCOLOR, NULL, (LPARAM)Drawing::ColorToWin32(this->ForeColor()));
		SendMessageA(this->_Handle, LVM_SETTEXTBKCOLOR, NULL, CLR_NONE);

		if (!Scrollable())
		{
			auto Style = (int32_t)GetWindowLong(this->_Handle, GWL_STYLE);
			Style |= LVS_NOSCROLL;
			SetWindowLong(this->_Handle, GWL_STYLE, Style);
		}

		SendMessageA(this->_Handle, LVM_SETVIEW, (WPARAM)_ViewStyle, NULL);

		auto ColumnCount = Columns.Count();
		if (ColumnCount > 0)
		{
			auto Indices = std::make_unique<int32_t[]>(ColumnCount);
			int32_t Index = 0;

			for (auto& Column : this->Columns)
			{
				Indices[Index] = Column.DisplayIndex();
				NativeInsertColumn(Index++, Column);
			}

			SetDisplayIndices(Indices, ColumnCount);
		}

		if (!VirtualMode())
			this->NativeInsertItems(0, Items.Count(), Items.ToList());

		if (VirtualMode() && VirtualListSize() > -1)
			SendMessageA(this->_Handle, LVM_SETITEMCOUNT, (WPARAM)VirtualListSize(), NULL);
	}

	void ListView::OnBackColorChanged()
	{
		SendMessageA(this->_Handle, LVM_SETBKCOLOR, NULL, (LPARAM)Drawing::ColorToWin32(this->BackColor()));

		// We must call the base event last
		Control::OnBackColorChanged();
	}

	void ListView::OnForeColorChanged()
	{
		SendMessageA(this->_Handle, LVM_SETTEXTCOLOR, NULL, (LPARAM)Drawing::ColorToWin32(this->ForeColor()));

		// We must call the base event last
		Control::OnForeColorChanged();
	}

	void ListView::OnFontChanged()
	{
		Control::OnFontChanged();

		if (!VirtualMode() && GetState(ControlStates::StateCreated))
			SendMessageA(this->_Handle, LVM_UPDATE, (WPARAM)-1, NULL);

		InvalidateColumnHeaders();
	}

	void ListView::OnItemActivate()
	{
		ItemActivate.RaiseEvent(this);
	}

	void ListView::OnColumnClick(const std::unique_ptr<ColumnClickEventArgs>& EventArgs)
	{
		ColumnClick.RaiseEvent(EventArgs, this);
	}

	void ListView::OnBeforeLabelEdit(const std::unique_ptr<LabelEditEventArgs>& EventArgs)
	{
		BeforeLabelEdit.RaiseEvent(EventArgs, this);
	}

	void ListView::OnAfterLabelEdit(const std::unique_ptr<LabelEditEventArgs>& EventArgs)
	{
		AfterLabelEdit.RaiseEvent(EventArgs, this);
	}

	void ListView::OnDrawItem(const std::unique_ptr<DrawListViewItemEventArgs>& EventArgs)
	{
		DrawItem.RaiseEvent(EventArgs, this);
	}

	void ListView::OnDrawSubItem(const std::unique_ptr<DrawListViewSubItemEventArgs>& EventArgs)
	{
		DrawSubItem.RaiseEvent(EventArgs, this);
	}

	void ListView::OnDrawColumnHeader(const std::unique_ptr<DrawListViewColumnHeaderEventArgs>& EventArgs)
	{
		DrawColumnHeader.RaiseEvent(EventArgs, this);
	}

	void ListView::OnCacheVirtualItems(const std::unique_ptr<CacheVirtualItemsEventArgs>& EventArgs)
	{
		CacheVirtualItems.RaiseEvent(EventArgs, this);
	}

	void ListView::OnRetrieveVirtualItem(const std::unique_ptr<RetrieveVirtualItemEventArgs>& EventArgs)
	{
		RetrieveVirtualItem.RaiseEvent(EventArgs, this);
	}

	void ListView::OnVirtualItemsSelectionRangeChanged(const std::unique_ptr<ListViewVirtualItemsSelectionRangeChangedEventArgs>& EventArgs)
	{
		VirtualItemsSelectionRangeChanged.RaiseEvent(EventArgs, this);
	}

	void ListView::WndProc(Message& Msg)
	{
		switch (Msg.Msg)
		{
		case WM_REFLECT + WM_NOTIFY:
			WmReflectNotify(Msg);
			break;
		case WM_LBUTTONDBLCLK:
			SetCapture(true);
			WmMouseDown(Msg, MouseButtons::Left, 2);
			break;
		case WM_LBUTTONDOWN:
			WmMouseDown(Msg, MouseButtons::Left, 1);
			_DownButton = MouseButtons::Left;
			break;

		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		{
			LVHITTESTINFO LvHip{};
			auto Index = GetIndexOfClickedItem(LvHip);

			if (GetFlag(ListViewFlags::FlagDoubleclickFired) && Index != -1)
			{
				SetFlag(ListViewFlags::FlagDoubleclickFired, false);
				OnDoubleClick();
				OnMouseDoubleClick(std::make_unique<MouseEventArgs>(_DownButton, 2, (int16_t)LOWORD(Msg.LParam), (int16_t)HIWORD(Msg.LParam), 0));
			}

			if (!GetFlag(ListViewFlags::FlagMouseUpFired))
			{
				OnMouseUp(std::make_unique<MouseEventArgs>(_DownButton, 1, (int16_t)LOWORD(Msg.LParam), (int16_t)HIWORD(Msg.LParam), 0));
				SetFlag(ListViewFlags::FlagExpectingMouseUp, false);
			}

			SetFlag(ListViewFlags::FlagMouseUpFired, true);
			SetCapture(false);
		}
		break;

		case WM_MBUTTONDBLCLK:
			WmMouseDown(Msg, MouseButtons::Middle, 2);
			break;
		case WM_MBUTTONDOWN:
			WmMouseDown(Msg, MouseButtons::Middle, 1);
			_DownButton = MouseButtons::Middle;
			break;
		case WM_RBUTTONDBLCLK:
			WmMouseDown(Msg, MouseButtons::Right, 2);
			break;
		case WM_RBUTTONDOWN:
			WmMouseDown(Msg, MouseButtons::Right, 1);
			_DownButton = MouseButtons::Right;
			break;
		case WM_MOUSEMOVE:
			if (GetFlag(ListViewFlags::FlagExpectingMouseUp) && !GetFlag(ListViewFlags::FlagMouseUpFired) && GetMouseButtons() == MouseButtons::None)
			{
				OnMouseUp(std::make_unique<MouseEventArgs>(_DownButton, 1, (int16_t)LOWORD(Msg.LParam), (int16_t)HIWORD(Msg.LParam), 0));
				SetFlag(ListViewFlags::FlagMouseUpFired, true);
			}

			SetCapture(false);
			Control::WndProc(Msg);
			break;
		case WM_NOTIFY:
			if (WmNotify(Msg))
				return;

			Control::WndProc(Msg);
			break;
		case WM_MOUSEHOVER:
			if (HoverSelection())
				Control::WndProc(Msg);
			else
				OnMouseHover();
			break;

		default:
			Control::WndProc(Msg);
			break;
		}
	}

	void ListView::CreateControl(Control* Parent)
	{
		INITCOMMONCONTROLSEX iCC{};
		iCC.dwSize = sizeof(iCC);
		iCC.dwICC = ICC_LISTVIEW_CLASSES;

		InitCommonControlsEx(&iCC);

		Control::CreateControl(Parent);
	}

	CreateParams ListView::GetCreateParams()
	{
		auto Cp = Control::GetCreateParams();

		Cp.ClassName = "SysListView32";

		if (GetState(ControlStates::StateCreated))
		{
			int32_t CurrentStyle = (int32_t)GetWindowLong(this->_Handle, GWL_STYLE);
			Cp.Style |= (CurrentStyle & (WS_HSCROLL | WS_VSCROLL));
		}

		Cp.Style |= LVS_SHAREIMAGELISTS;

		switch (_ListViewAlignment)
		{
		case ListViewAlignment::Top:
			Cp.Style |= LVS_ALIGNTOP;
			break;
		case ListViewAlignment::Left:
			Cp.Style |= LVS_ALIGNLEFT;
			break;
		}

		if (AutoArrange())
			Cp.Style |= LVS_AUTOARRANGE;

		switch (_BorderStyle)
		{
		case BorderStyle::Fixed3D:
			Cp.ExStyle |= WS_EX_CLIENTEDGE;
			break;
		case BorderStyle::FixedSingle:
			Cp.Style |= WS_BORDER;
			break;
		}

		switch (_ColumnHeaderStyle)
		{
		case ColumnHeaderStyle::None:
			Cp.Style |= LVS_NOCOLUMNHEADER;
			break;
		case ColumnHeaderStyle::NonClickable:
			Cp.Style |= LVS_NOSORTHEADER;
			break;
		}

		if (LabelEdit())
			Cp.Style |= LVS_EDITLABELS;
		if (!LabelWrap())
			Cp.Style |= LVS_NOLABELWRAP;
		if (!HideSelection())
			Cp.Style |= LVS_SHOWSELALWAYS;
		if (!MultiSelect())
			Cp.Style |= LVS_SINGLESEL;

		switch (_SortOrder)
		{
		case SortOrder::Ascending:
			Cp.Style |= LVS_SORTASCENDING;
			break;
		case SortOrder::Descending:
			Cp.Style |= LVS_SORTDESCENDING;
			break;
		}

		if (VirtualMode())
			Cp.Style |= LVS_OWNERDATA;

		if (_ViewStyle != View::Tile)
			Cp.Style |= (int32_t)_ViewStyle;

		return Cp;
	}

	void ListView::ScaleControl(Drawing::SizeF Factor, BoundsSpecified Specified)
	{
		Control::ScaleControl(Factor, Specified);

		// We override this so we can scale the current column widths
		for (auto& Column : this->Columns)
		{
			Column.SetWidth(this->ScaleSize({ Column.Width(), 0 }, Factor.Width, Factor.Height).Width);
		}
	}

	bool ListView::GetFlag(ListViewFlags Flag)
	{
		return ((int)this->_Flags & (int)Flag) == (int)Flag;
	}

	void ListView::SetFlag(ListViewFlags Flags, bool Value)
	{
		this->_Flags = Value ? (ListViewFlags)((int)this->_Flags | (int)Flags) : (ListViewFlags)((int)this->_Flags & ~(int)Flags);
	}

	void ListView::NativeAddColumn(ColumnHeader& Header)
	{
		NativeInsertColumn(this->Columns.Count(), Header);
	}

	void ListView::NativeInsertColumn(const int32_t Index, ColumnHeader& Header)
	{
		LVCOLUMNA LvColumn{};
		LvColumn.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;

		LvColumn.fmt = (int32_t)Header.TextAlign();
		LvColumn.cx = Header.Width();
		LvColumn.pszText = (char*)Header.Text();

		SendMessageA(this->_Handle, LVM_INSERTCOLUMNA, (WPARAM)Index, (LPARAM)&LvColumn);
	}

	void ListView::NativeClearColumns()
	{
		for (int32_t ColIdx = this->Columns.Count() - 1; ColIdx >= 0; ColIdx--)
			SendMessageA(this->_Handle, LVM_DELETECOLUMN, (WPARAM)ColIdx, NULL);
	}

	void ListView::NativeInsertItem(int32_t Index, const ListViewItem& Item)
	{
		SendMessageA(this->_Handle, LVM_SETITEMCOUNT, (WPARAM)Items.Count(), NULL);

		LVITEMA LvItem{};
		LvItem.mask = LVIF_TEXT;
		LvItem.iItem = Index;
		LvItem.pszText = (char*)Item.Text();
		
		SendMessageA(this->_Handle, LVM_INSERTITEMA, NULL, (LPARAM)&LvItem);

		for (uint32_t i = 1; i <= Item.SubItemCount(); i++)
		{
			LVITEMA LvSub{};
			LvSub.mask = LVIF_TEXT;
			LvSub.iItem = Index;
			LvSub.iSubItem = i;
			LvSub.pszText = (char*)Item.SubItem(i);

			SendMessageA(this->_Handle, LVM_SETITEMTEXTA, (WPARAM)Index, (LPARAM)&LvSub);
		}
	}

	void ListView::NativeInsertItems(int32_t Index, int32_t Count, const List<ListViewItem>& Items)
	{
		SendMessageA(this->_Handle, LVM_SETITEMCOUNT, (WPARAM)Items.Count(), NULL);

		for (int32_t i = 0; i < Count; i++)
		{
			auto& Item = Items[Index + i];

			LVITEMA LvItem{};
			LvItem.mask = LVIF_TEXT;
			LvItem.iItem = Index + i;
			LvItem.pszText = (char*)Item.Text();

			SendMessageA(this->_Handle, LVM_INSERTITEMA, NULL, (LPARAM)&LvItem);

			for (uint32_t j = 1; j <= Item.SubItemCount(); j++)
			{
				LVITEMA LvSub{};
				LvSub.mask = LVIF_TEXT;
				LvSub.iItem = Index + i;
				LvSub.iSubItem = j;
				LvSub.pszText = (char*)Item.SubItem(j);

				SendMessageA(this->_Handle, LVM_SETITEMTEXTA, (WPARAM)(Index + i), (LPARAM)&LvSub);
			}
		}
	}

	void ListView::NativeRemoveItem(int32_t Index)
	{
		SendMessageA(this->_Handle, LVM_DELETEITEM, (WPARAM)Index, NULL);
	}

	void ListView::NativeClearItems()
	{
		SendMessageA(this->_Handle, LVM_DELETEALLITEMS, NULL, NULL);
	}

	void ListView::InvalidateColumnHeaders()
	{
		if (GetState(ControlStates::StateCreated) && _ViewStyle == View::Details)
		{
			auto HwndHdr = (HWND)SendMessageA(this->_Handle, LVM_GETHEADER, NULL, NULL);
			if (HwndHdr != NULL)
				InvalidateRect(HwndHdr, NULL, TRUE);
		}
	}

	Drawing::Point ListView::GetItemPosition(int32_t Index)
	{
		POINT Pt{};
		SendMessageA(this->_Handle, LVM_GETITEMPOSITION, (WPARAM)Index, (LPARAM)&Pt);

		return Drawing::Point(Pt.x, Pt.y);
	}

	Drawing::Rectangle ListView::GetItemRectOrEmpty(int32_t Index)
	{
		if (Index < 0 || Index >= (int32_t)this->Items.Count())
			return Drawing::Rectangle();
		if (_ViewStyle == View::Details && this->Columns.Count() == 0)
			return Drawing::Rectangle();

		RECT ItemRect{};
		if (SendMessageA(this->_Handle, LVM_GETITEMRECT, (WPARAM)Index, (LPARAM)&ItemRect) == 0)
			return Drawing::Rectangle();

		return Drawing::Rectangle(ItemRect.left, ItemRect.top, ItemRect.right - ItemRect.left, ItemRect.bottom - ItemRect.top);
	}

	Drawing::Rectangle ListView::GetSubItemRect(int32_t Index, int32_t SubItem)
	{
		if (_ViewStyle != View::Details)
			return Drawing::Rectangle();
		if (this->Columns.Count() == 0)
			return Drawing::Rectangle();

		RECT ItemRect{};
		ItemRect.top = SubItem;

		if (SendMessageA(this->_Handle, LVM_GETSUBITEMRECT, (WPARAM)Index, (LPARAM)&ItemRect) == 0)
			return Drawing::Rectangle();

		return Drawing::Rectangle(ItemRect.left, ItemRect.top, ItemRect.right - ItemRect.left, ItemRect.bottom - ItemRect.top);
	}

	int32_t ListView::GetItemState(int32_t Index)
	{
		return (int32_t)SendMessageA(this->_Handle, LVM_GETITEMSTATE, (WPARAM)Index, LVIS_FOCUSED | LVIS_SELECTED | LVIS_CUT | LVIS_DROPHILITED | LVIS_OVERLAYMASK | LVIS_STATEIMAGEMASK);
	}

	std::pair<string, ListViewItemStyle> ListView::GetSubItem(int32_t Index, int32_t SubItem)
	{
		if (!VirtualMode())
		{
			auto& Item = Items[Index];
			auto& Text = Item.SubItem(SubItem);
			auto& Style = Item.SubItemStyle(SubItem);

			return std::pair<string, ListViewItemStyle>(Text, Style);
		}

		auto EventArgs = std::make_unique<RetrieveVirtualItemEventArgs>(Index, SubItem);
		OnRetrieveVirtualItem(EventArgs);

		return std::pair<string, ListViewItemStyle>(EventArgs->Text, EventArgs->Style);
	}

	void ListView::SetColumnInfo(int32_t Mask, ColumnHeader& Header)
	{
		if (!GetState(ControlStates::StateCreated))
			return;

		LVCOLUMNA LvColumn{};
		LvColumn.mask = Mask;

		if ((Mask & LVCF_FMT) != 0)
		{
			LvColumn.fmt |= (int32_t)Header.TextAlign();
		}

		if ((Mask & LVCF_TEXT) != 0)
		{
			LvColumn.pszText = (char*)Header.Text();
		}

		SendMessageA(this->_Handle, LVM_SETCOLUMNA, (WPARAM)Header.Index(), (LPARAM)&LvColumn);
		InvalidateColumnHeaders();
	}

	void ListView::SetColumnWidth(int32_t Index, int32_t Width)
	{
		if (GetState(ControlStates::StateCreated))
			SendMessageA(this->_Handle, LVM_SETCOLUMNWIDTH, (WPARAM)Index, MAKELPARAM(Width, 0));
	}

	void ListView::SetDisplayIndices(const std::unique_ptr<int32_t[]>& Indices, int32_t Count)
	{
		auto OrderedColumns = std::make_unique<int32_t[]>(Count);
		for (int32_t i = 0; i < Count; i++)
		{
			this->Columns[i].SetDisplayIndexInternal(Indices[i]);
			OrderedColumns[Indices[i]] = i;
		}

		if (GetState(ControlStates::StateCreated))
			SendMessageA(this->_Handle, LVM_SETCOLUMNORDERARRAY, (WPARAM)Count, (LPARAM)OrderedColumns.get());
	}

	void ListView::UpdateExtendedStyles()
	{
		if (GetState(ControlStates::StateCreated))
		{
			int32_t ExStyle = 0;
			int32_t ExMask = LVS_EX_ONECLICKACTIVATE | LVS_EX_TWOCLICKACTIVATE |
				LVS_EX_TRACKSELECT | LVS_EX_UNDERLINEHOT | LVS_EX_ONECLICKACTIVATE |
				LVS_EX_HEADERDRAGDROP | LVS_EX_CHECKBOXES |
				LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES |
				LVS_EX_INFOTIP | LVS_EX_DOUBLEBUFFER;

			switch (_ItemActivation)
			{
			case ItemActivation::OneClick:
				ExStyle |= LVS_EX_ONECLICKACTIVATE;
				break;
			case ItemActivation::TwoClick:
				ExStyle |= LVS_EX_TWOCLICKACTIVATE;
				break;
			}

			if (AllowColumnReorder())
				ExStyle |= LVS_EX_HEADERDRAGDROP;
			if (DoubleBuffered())
				ExStyle |= LVS_EX_DOUBLEBUFFER;
			if (FullRowSelect())
				ExStyle |= LVS_EX_FULLROWSELECT;
			if (GridLines())
				ExStyle |= LVS_EX_GRIDLINES;
			if (HoverSelection())
				ExStyle |= LVS_EX_TRACKSELECT;
			if (HotTracking())
				ExStyle |= LVS_EX_UNDERLINEHOT;

			SendMessageA(this->_Handle, LVM_SETEXTENDEDLISTVIEWSTYLE, (WPARAM)ExMask, (LPARAM)ExStyle);
			Invalidate();
		}
	}

	int32_t ListView::GetIndexOfClickedItem(LVHITTESTINFO & LvHI)
	{
		auto Pos = GetMousePosition();
		Pos = PointToClient(Pos);

		LvHI.pt.x = Pos.X;
		LvHI.pt.y = Pos.Y;

		return (int32_t)SendMessageA(this->_Handle, LVM_HITTEST, NULL, (LPARAM)&LvHI);
	}

	void ListView::WmMouseDown(Message& Msg, MouseButtons Button, uint32_t Clicks)
	{
		SetFlag(ListViewFlags::FlagMouseUpFired, false);
		SetFlag(ListViewFlags::FlagExpectingMouseUp, true);

		this->Focus();

		int32_t X = LOWORD((int)Msg.LParam);
		int32_t Y = HIWORD((int)Msg.LParam);
		auto EventArgs = std::make_unique<MouseEventArgs>(Button, Clicks, X, Y, 0);

		OnMouseDown(EventArgs);

		DefWndProc(Msg);
	}

	void ListView::WmReflectNotify(Message& Msg)
	{
		auto NMHdr = (NMHDR*)Msg.LParam;

		switch (NMHdr->code)
		{
		case NM_CUSTOMDRAW:
			NmCustomDraw(Msg);
			break;

		case LVN_BEGINLABELEDITA:
		case LVN_BEGINLABELEDITW:
		{
			auto DispInfo = (NMLVDISPINFOA*)Msg.LParam;
			auto EventArgs = std::make_unique<LabelEditEventArgs>(DispInfo->item.iItem);
			OnBeforeLabelEdit(EventArgs);
			Msg.Result = (uintptr_t)(EventArgs->CancelEdit ? 1 : 0);
			SetFlag(ListViewFlags::FlagInLabelEdit, !EventArgs->CancelEdit);
		}
		break;

		case LVN_COLUMNCLICK:
			SetFlag(ListViewFlags::FlagColumnClicked, true);
			_ColumnIndex = ((NMLISTVIEW*)Msg.LParam)->iSubItem;
			break;

		case LVN_ENDLABELEDITA:
		case LVN_ENDLABELEDITW:
		{
			auto DispInfo = (NMLVDISPINFOA*)Msg.LParam;
			auto EventArgs = std::make_unique<LabelEditEventArgs>(DispInfo->item.iItem, DispInfo->item.pszText);
			OnAfterLabelEdit(EventArgs);
			Msg.Result = (uintptr_t)(EventArgs->CancelEdit ? 1 : 0);

			if (!EventArgs->CancelEdit && DispInfo->item.pszText != nullptr)
			{
				LVITEMA LvItem{};
				LvItem.mask = LVIF_TEXT;
				LvItem.iItem = DispInfo->item.iItem;
				LvItem.pszText = DispInfo->item.pszText;
				SendMessageA(this->_Handle, LVM_SETITEMTEXTA, (WPARAM)DispInfo->item.iItem, (LPARAM)&LvItem);
			}
		}
		break;

		case LVN_ITEMACTIVATE:
			OnItemActivate();
			break;

		case NM_CLICK:
		case NM_RCLICK:
		{
			LVHITTESTINFO LvHI{};
			auto DisplayIndex = GetIndexOfClickedItem(LvHI);
			auto Button = NMHdr->code == NM_CLICK ? MouseButtons::Left : MouseButtons::Right;
			auto Pos = GetMousePosition();
			Pos = PointToClient(Pos);

			if (DisplayIndex != -1)
			{
				OnClick();
				OnMouseClick(std::make_unique<MouseEventArgs>(Button, 1, Pos.X, Pos.Y, 0));
			}
			if (!GetFlag(ListViewFlags::FlagMouseUpFired))
			{
				OnMouseUp(std::make_unique<MouseEventArgs>(Button, 1, Pos.X, Pos.Y, 0));
				SetFlag(ListViewFlags::FlagMouseUpFired, true);
			}
		}
		break;

		case NM_DBLCLK:
		case NM_RDBLCLK:
		{
			LVHITTESTINFO LvHI{};
			auto DisplayIndex = GetIndexOfClickedItem(LvHI);

			if (DisplayIndex != -1)
				SetFlag(ListViewFlags::FlagDoubleclickFired, true);
			SetFlag(ListViewFlags::FlagMouseUpFired, false);

			SetCapture(true);
		}
		break;

		case LVN_ODCACHEHINT:
		{
			auto CacheHint = (NMLVCACHEHINT*)Msg.LParam;
			OnCacheVirtualItems(std::make_unique<CacheVirtualItemsEventArgs>(CacheHint->iFrom, CacheHint->iTo));
		}
		break;

		case LVN_GETDISPINFOA:
		case LVN_GETDISPINFOW:
			if (VirtualMode())
			{
				auto DispInfo = (NMLVDISPINFOA*)Msg.LParam;

				if ((DispInfo->item.mask & LVIF_TEXT) != 0)
				{
					auto EventArgs = std::make_unique<RetrieveVirtualItemEventArgs>(DispInfo->item.iItem, DispInfo->item.iSubItem);
					OnRetrieveVirtualItem(EventArgs);

					auto TextSize = min(EventArgs->Text.Length(), (uint32_t)(DispInfo->item.cchTextMax - 1));
					std::memcpy(DispInfo->item.pszText, (char*)EventArgs->Text, (size_t)TextSize);
					DispInfo->item.pszText[TextSize] = '\0';
				}
			}
			break;
			
		case LVN_ODSTATECHANGED:
		{
			auto OdStateChange = (NMLVODSTATECHANGE*)Msg.LParam;
			if ((OdStateChange->uNewState & LVIS_SELECTED) != (OdStateChange->uOldState & LVIS_SELECTED))
			{
				OnVirtualItemsSelectionRangeChanged(std::make_unique<ListViewVirtualItemsSelectionRangeChangedEventArgs>(OdStateChange->iFrom, OdStateChange->iTo, (OdStateChange->uNewState & LVIS_SELECTED) != 0));
			}
		}
		break;

		case LVN_ITEMCHANGED:
		{
			auto ChangeInfo = (NMLISTVIEW*)Msg.LParam;
			if ((ChangeInfo->uChanged & LVIF_STATE) != 0)
			{
				int32_t oldState = ChangeInfo->uOldState & LVIS_SELECTED;
				int32_t newState = ChangeInfo->uNewState & LVIS_SELECTED;

				if (newState != oldState)
				{
					// Trigger virtual item selection change here as well...
					if (this->VirtualListSize() > 0)
					{
						OnVirtualItemsSelectionRangeChanged(std::make_unique<ListViewVirtualItemsSelectionRangeChangedEventArgs>(0, this->VirtualListSize() - 1, newState != 0));
					}

					// TODO: Trigger OnSelectedIndexChanged();
					// https://referencesource.microsoft.com/#System.Windows.Forms/winforms/Managed/System/WinForms/ListView.cs,5946
				}
			}
		}
		break;
		}
	}

	bool ListView::WmNotify(Message& Msg)
	{
		auto NmHdr = (NMHDR*)Msg.LParam;

		if (NmHdr->code == NM_CUSTOMDRAW && OwnerDraw())
		{
			auto NmCd = (NMCUSTOMDRAW*)Msg.LParam;

			switch (NmCd->dwDrawStage)
			{
			case CDDS_PREPAINT:
				Msg.Result = (uintptr_t)CDRF_NOTIFYITEMDRAW;
				return true;

			case CDDS_ITEMPREPAINT:
			{
				auto EventArgs = std::make_unique<DrawListViewColumnHeaderEventArgs>((HDC)NmCd->hdc, &Columns[(uint32_t)NmCd->dwItemSpec], (int32_t)NmCd->dwItemSpec, Drawing::Rectangle(NmCd->rc.left, NmCd->rc.top, NmCd->rc.right - NmCd->rc.left, NmCd->rc.bottom - NmCd->rc.top), (ListViewItemStates)NmCd->uItemState);
				OnDrawColumnHeader(EventArgs);

				if (EventArgs->DrawDefault)
				{
					Msg.Result = (uintptr_t)CDRF_DODEFAULT;
					return false;
				}
				else
				{
					Msg.Result = (uintptr_t)CDRF_SKIPDEFAULT;
					return true;
				}
			}
			break;

			default:
				return false;
			}
		}

		if (NmHdr->code == NM_RELEASEDCAPTURE && GetFlag(ListViewFlags::FlagColumnClicked))
		{
			SetFlag(ListViewFlags::FlagColumnClicked, false);
			OnColumnClick(std::make_unique<ColumnClickEventArgs>(_ColumnIndex));
		}

		// Still need default handling
		return false;
	}

	void ListView::NmCustomDraw(Message& Msg)
	{
		bool DontMess = false;
		bool ItemDrawDefault = false;

		auto NmCd = (NMLVCUSTOMDRAW*)Msg.LParam;

		switch (NmCd->nmcd.dwDrawStage)
		{
		case CDDS_PREPAINT:
			if (OwnerDraw())
			{
				Msg.Result = (uintptr_t)CDRF_NOTIFYITEMDRAW;
				return;
			}

			Msg.Result = (uintptr_t)CDRF_NOTIFYSUBITEMDRAW;
			return;

		case CDDS_ITEMPREPAINT:
		{
			int32_t ItemIndex = (int32_t)NmCd->nmcd.dwItemSpec;
			auto ItemBounds = GetItemRectOrEmpty(ItemIndex);

			if (!ClientRectangle().IntersectsWith(ItemBounds))
				return;

			if (OwnerDraw())
			{
				auto Item = this->GetSubItem(ItemIndex, 0);
				auto EventArgs = std::make_unique<DrawListViewItemEventArgs>((HDC)NmCd->nmcd.hdc, Item.first, Item.second, ItemBounds, ItemIndex, (ListViewItemStates)NmCd->nmcd.uItemState);
				OnDrawItem(EventArgs);

				ItemDrawDefault = EventArgs->DrawDefault;

				if (_ViewStyle == View::Details)
					Msg.Result = (uintptr_t)CDRF_NOTIFYSUBITEMDRAW;
				else if (!EventArgs->DrawDefault)
					Msg.Result = (uintptr_t)CDRF_SKIPDEFAULT;

				if (!EventArgs->DrawDefault)
					return;
			}

			if (_ViewStyle == View::Details || _ViewStyle == View::Tile)
			{
				Msg.Result = (uintptr_t)CDRF_NOTIFYSUBITEMDRAW;
				DontMess = true;
			}
		}

		case (CDDS_SUBITEM | CDDS_ITEMPREPAINT):
		{
			int32_t ItemIndex = (int32_t)NmCd->nmcd.dwItemSpec;
			auto ItemBounds = GetItemRectOrEmpty(ItemIndex);

			if (!ClientRectangle().IntersectsWith(ItemBounds))
				return;

			if (OwnerDraw() && !ItemDrawDefault)
			{
				bool SkipCustomDrawCode = true;

				if (NmCd->iSubItem < (int32_t)Columns.Count())
				{
					auto SubItemBounds = GetSubItemRect(ItemIndex, NmCd->iSubItem);

					if (NmCd->iSubItem == 0 && Columns.Count() > 1)
					{
						SubItemBounds.Width = Columns[0].Width();
					}

					if (ClientRectangle().IntersectsWith(SubItemBounds))
					{
						auto Item = this->GetSubItem(ItemIndex, NmCd->iSubItem);
						auto EventArgs = std::make_unique<DrawListViewSubItemEventArgs>((HDC)NmCd->nmcd.hdc, Item.first, Item.second, SubItemBounds, ItemIndex, NmCd->iSubItem, (ListViewItemStates)NmCd->nmcd.uItemState);
						OnDrawSubItem(EventArgs);

						SkipCustomDrawCode = !EventArgs->DrawDefault;
					}
				}

				if (SkipCustomDrawCode)
				{
					Msg.Result = (uintptr_t)CDRF_SKIPDEFAULT;
					return;
				}
			}

			auto State = NmCd->nmcd.uItemState;
			if (!HideSelection())
			{
				auto RealState = GetItemState((int32_t)NmCd->nmcd.dwItemSpec);
				if ((RealState & LVIS_SELECTED) == 0)
					State &= ~CDIS_SELECTED;
			}

			auto SubItem = ((NmCd->nmcd.dwDrawStage & CDDS_SUBITEM) != 0) ? NmCd->iSubItem : 0;
			auto Item = GetSubItem(ItemIndex, SubItem);

			bool ChangeColor = (GetState(ControlStates::StateEnabled) ? true : false);
			if (Activation() == ItemActivation::OneClick || Activation() == ItemActivation::TwoClick)
			{
				if ((State & (CDIS_SELECTED | CDIS_GRAYED | CDIS_HOT | CDIS_DISABLED)) != 0)
					ChangeColor = false;
			}

			if (ChangeColor)
			{
				NmCd->clrText = Drawing::ColorToWin32(Item.second.ForeColor);
				NmCd->clrTextBk = Drawing::ColorToWin32(Item.second.BackColor);
			}

			if (!DontMess)
				Msg.Result = (uintptr_t)CDRF_NEWFONT;
		}
		return;

		default:
			Msg.Result = (uintptr_t)CDRF_DODEFAULT;
			return;
		}
	}

	ListView::ColumnHeaderCollection::ColumnHeaderCollection(ListView* Owner)
		: _Owner(Owner), _Columns()
	{
	}

	void ListView::ColumnHeaderCollection::Add(const ColumnHeader& Value)
	{
		auto& Result = _Columns.Emplace(Value);

		Result.SetListView(_Owner);
		
		if (Result.DisplayIndex() <= -1)
			Result.SetDisplayIndexInternal(_Columns.Count() - 1);

		if (_Owner->GetState(ControlStates::StateCreated))
			_Owner->NativeAddColumn(Result);
	}

	void ListView::ColumnHeaderCollection::Insert(int32_t Index, const ColumnHeader& Value)
	{
		_Columns.Insert(Index, Value);

		auto& Result = _Columns[Index];

		Result.SetListView(_Owner);

		if (Result.DisplayIndex() <= -1)
			Result.SetDisplayIndexInternal(Index);

		if (_Owner->GetState(ControlStates::StateCreated))
			_Owner->NativeInsertColumn(Index, Result);
	}

	void ListView::ColumnHeaderCollection::Clear()
	{
		_Owner->NativeClearColumns();
		_Columns.Clear();
	}

	int32_t ListView::ColumnHeaderCollection::IndexOf(const ColumnHeader& Value)
	{
		return this->_Columns.IndexOf(Value);
	}

	uint32_t ListView::ColumnHeaderCollection::Count()
	{
		return this->_Columns.Count();
	}

	ColumnHeader* ListView::ColumnHeaderCollection::begin() const
	{
		return this->_Columns.begin();
	}

	ColumnHeader* ListView::ColumnHeaderCollection::end() const
	{
		return this->_Columns.end();
	}

	ColumnHeader& ListView::ColumnHeaderCollection::operator[](uint32_t Index)
	{
		return this->_Columns[Index];
	}

	ListView::ListViewItemCollection::ListViewItemCollection(ListView* Owner)
		: _Owner(Owner)
	{
	}

	void ListView::ListViewItemCollection::Add(const ListViewItem& Value)
	{
		_Items.EmplaceBack(Value);

		if (_Owner->GetState(ControlStates::StateCreated))
			_Owner->NativeInsertItem(_Items.Count(), Value);
	}

	void ListView::ListViewItemCollection::Insert(int32_t Index, const ListViewItem& Value)
	{
		_Items.Insert(Index, Value);

		if (_Owner->GetState(ControlStates::StateCreated))
			_Owner->NativeInsertItem(Index, Value);
	}

	void ListView::ListViewItemCollection::RemoveAt(int32_t Index)
	{
		_Items.RemoveAt(Index);

		if (_Owner->GetState(ControlStates::StateCreated))
			_Owner->NativeRemoveItem(Index);
	}

	void ListView::ListViewItemCollection::Clear()
	{
		_Items.Clear();

		if (_Owner->GetState(ControlStates::StateCreated))
			_Owner->NativeClearItems();
	}

	uint32_t ListView::ListViewItemCollection::Count()
	{
		return (_Owner->VirtualMode() ? (uint32_t)_Owner->_VirtualListSize : this->_Items.Count());
	}

	ListViewItem& ListView::ListViewItemCollection::operator[](uint32_t Index)
	{
		return _Items[Index];
	}

	ListViewItem* ListView::ListViewItemCollection::begin() const
	{
		return _Items.begin();
	}

	ListViewItem* ListView::ListViewItemCollection::end() const
	{
		return _Items.end();
	}

	const List<ListViewItem>& ListView::ListViewItemCollection::ToList()
	{
		return _Items;
	}
}
