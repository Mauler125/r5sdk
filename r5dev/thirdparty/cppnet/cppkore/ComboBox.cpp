#include "stdafx.h"
#include "ComboBox.h"

namespace Forms
{
	ComboBox::ComboBox()
		: Control(), Items(this), _DrawMode(DrawMode::Normal), _FlatStyle(FlatStyle::Standard), _DropDownStyle(ComboBoxStyle::DropDown), _IntegralHeight(true), _DropDownWidth(-1), _DropDownHeight(-1), _MaxDropDownItems(8), _MaximumLength(SHRT_MAX), _SelectedIndex(-1), _ChildEdit(nullptr), _ChildListBox(nullptr)
	{
		SetStyle(ControlStyles::UserPaint |
			ControlStyles::UseTextForAccessibility |
			ControlStyles::StandardClick, false);

		// Default back color is different...
		this->_BackColor = Drawing::GetSystemColor(Drawing::SystemColors::Window);
		// We are a ComboBox control.
		this->_RTTI = ControlTypes::ComboBox;
	}

	DrawMode ComboBox::GetDrawMode()
	{
		return this->_DrawMode;
	}

	void ComboBox::SetDrawMode(DrawMode Value)
	{
		if (_DrawMode != Value)
		{
			_DrawMode = Value;
			UpdateStyles();
		}
	}

	uint32_t ComboBox::DropDownWidth()
	{
		if (_DropDownWidth > -1)
			return _DropDownWidth;

		return _Width;
	}

	void ComboBox::SetDropDownWidth(uint32_t Value)
	{
		_DropDownWidth = (int32_t)Value;

		if (GetState(ControlStates::StateCreated))
			SendMessageA(this->_Handle, CB_SETDROPPEDWIDTH, (WPARAM)Value, NULL);
	}

	uint32_t ComboBox::DropDownHeight()
	{
		if (_DropDownHeight > -1)
			return _DropDownHeight;

		return 106;	// Default drop down height...
	}

	void ComboBox::SetDropDownHeight(uint32_t Value)
	{
		_DropDownHeight = (int32_t)Value;
		SetIntegralHeight(false);
	}

	bool ComboBox::DroppedDown()
	{
		if (GetState(ControlStates::StateCreated))
			return (SendMessageA(this->_Handle, CB_GETDROPPEDSTATE, NULL, NULL) != 0);

		return false;
	}

	void ComboBox::SetDroppedDown(bool Value)
	{
		if (GetState(ControlStates::StateCreated))
			SendMessageA(this->_Handle, CB_SHOWDROPDOWN, Value ? -1 : 0, NULL);
	}

	FlatStyle ComboBox::GetFlatStyle()
	{
		return this->_FlatStyle;
	}

	void ComboBox::SetFlatStyle(FlatStyle Value)
	{
		if (_FlatStyle != Value)
		{
			_FlatStyle = Value;
			UpdateStyles();
		}
	}

	bool ComboBox::IntegralHeight()
	{
		return this->_IntegralHeight;
	}

	void ComboBox::SetIntegralHeight(bool Value)
	{
		if (_IntegralHeight != Value)
		{
			_IntegralHeight = Value;
			UpdateStyles();
		}
	}

	int32_t ComboBox::ItemHeight()
	{
		return (int32_t)SendMessageA(this->_Handle, CB_GETITEMHEIGHT, NULL, NULL);
	}

	void ComboBox::SetItemHeight(int32_t Value)
	{
		if (_DrawMode == DrawMode::OwnerDrawFixed)
		{
			SendMessageA(this->_Handle, CB_SETITEMHEIGHT, (WPARAM)-1, (LPARAM)Value);
			SendMessageA(this->_Handle, CB_SETITEMHEIGHT, (WPARAM)0, (LPARAM)Value);
		}
		else if (_DrawMode == DrawMode::OwnerDrawVariable)
		{
			SendMessageA(this->_Handle, CB_SETITEMHEIGHT, (WPARAM)-1, (LPARAM)Value);

			for (uint32_t i = 0; i < Items.Count(); i++)
				SendMessageA(this->_Handle, CB_SETITEMHEIGHT, (WPARAM)i, (LPARAM)Value);
		}
	}

	uint8_t ComboBox::MaxDropDownItems()
	{
		return this->_MaxDropDownItems;
	}

	void ComboBox::SetMaxDropDownItems(uint8_t Value)
	{
		this->_MaxDropDownItems = Value;
	}

	uint32_t ComboBox::MaxLength()
	{
		return this->_MaximumLength;
	}

	void ComboBox::SetMaxLength(uint32_t Value)
	{
		if (_MaximumLength != Value)
		{
			_MaximumLength = Value;
			
			if (GetState(ControlStates::StateCreated))
				SendMessageA(this->_Handle, CB_LIMITTEXT, (WPARAM)Value, NULL);
		}
	}

	int32_t ComboBox::SelectedIndex()
	{
		if (GetState(ControlStates::StateCreated))
			return (int32_t)SendMessageA(this->_Handle, CB_GETCURSEL, NULL, NULL);

		return this->_SelectedIndex;
	}

	void ComboBox::SetSelectedIndex(int32_t Value)
	{
		if (GetState(ControlStates::StateCreated))
			SendMessageA(this->_Handle, CB_SETCURSEL, (WPARAM)Value, NULL);
		else
			_SelectedIndex = Value;

		OnTextChanged();

		OnSelectedIndexChanged();
		OnSelectedItemChanged();
	}

	String ComboBox::SelectedText()
	{
		if (_DropDownStyle == ComboBoxStyle::DropDownList)
			return "";

		return Text().SubString(SelectionStart(), SelectionLength());
	}

	void ComboBox::SetSelectedText(const String& Value)
	{
		if (_DropDownStyle != ComboBoxStyle::DropDownList)
		{
			if (GetState(ControlStates::StateCreated))
				SendMessageA(this->_ChildEdit, EM_REPLACESEL, (WPARAM)-1, (LPARAM)(const char*)Value);
		}
	}

	int32_t ComboBox::SelectionLength()
	{
		int32_t End = 0;
		int32_t Start = 0;
		SendMessageA(this->_Handle, CB_GETEDITSEL, (WPARAM)&Start, (WPARAM)&End);

		return End - Start;
	}

	void ComboBox::SetSelectionLength(int32_t Value)
	{
		Select(SelectionStart(), Value);
	}

	int32_t ComboBox::SelectionStart()
	{
		int32_t Value = 0;
		SendMessageA(this->_Handle, CB_GETEDITSEL, (WPARAM)&Value, NULL);

		return Value;
	}

	void ComboBox::SetSelectionStart(int32_t Value)
	{
		Select(Value, SelectionLength());
	}

	ComboBoxStyle ComboBox::DropDownStyle()
	{
		return this->_DropDownStyle;
	}

	void ComboBox::SetDropDownStyle(ComboBoxStyle Value)
	{
		if (_DropDownStyle != Value)
		{
			_DropDownStyle = Value;

			if (GetState(ControlStates::StateCreated))
				UpdateStyles();
		}
	}

	void ComboBox::Select(int32_t Start, int32_t Length)
	{
		int32_t End = Start + Length;
		SendMessageA(this->_Handle, CB_SETEDITSEL, NULL, MAKELPARAM(Start, End));
	}

	void ComboBox::OnHandleCreated()
	{
		if (_MaximumLength > 0)
			SendMessageA(this->_Handle, CB_LIMITTEXT, (WPARAM)_MaximumLength, NULL);

		if (_DropDownStyle != ComboBoxStyle::DropDownList)
		{
			auto Hwnd = GetWindow(this->_Handle, GW_CHILD);
			if (Hwnd != NULL)
			{
				if (_DropDownStyle == ComboBoxStyle::Simple)
				{
					_ChildListBox = Hwnd;
					Hwnd = GetWindow(Hwnd, GW_HWNDNEXT);
				}

				_ChildEdit = Hwnd;
			}
		}

		if (_DropDownWidth > -1)
			SendMessageA(this->_Handle, CB_SETDROPPEDWIDTH, (WPARAM)_DropDownWidth, NULL);

		// If we have items, add them now
		for (auto& Item : Items)
			this->NativeAdd((const char*)Item);

		if (_SelectedIndex > -1)
		{
			SendMessageA(this->_Handle, CB_SETCURSEL, (WPARAM)_SelectedIndex, NULL);
			_SelectedIndex = -1;
		}

		// We must call the base event last
		Control::OnHandleCreated();
	}

	void ComboBox::OnSelectedItemChanged()
	{
		SelectedItemChanged.RaiseEvent(this);
	}

	void ComboBox::OnSelectedIndexChanged()
	{
		SelectedIndexChanged.RaiseEvent(this);
	}

	void ComboBox::OnDropDownOpened()
	{
		DropDownOpened.RaiseEvent(this);
	}

	void ComboBox::OnDropDownClosed()
	{
		DropDownClosed.RaiseEvent(this);
	}

	void ComboBox::WndProc(Message& Msg)
	{
		switch (Msg.Msg)
		{
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORLISTBOX:
			Msg.Result = InitializeDCForWmCtlColor((HDC)Msg.WParam, Msg.Msg);
			break;
		case WM_REFLECT + WM_COMMAND:
			WmReflectCommand(Msg);
			break;
		default:
			Control::WndProc(Msg);
			break;
		}
	}

	CreateParams ComboBox::GetCreateParams()
	{
		auto Cp = Control::GetCreateParams();

		Cp.ClassName = "COMBOBOX";
		Cp.Style |= WS_VSCROLL | CBS_HASSTRINGS | CBS_AUTOHSCROLL;

		if (!_IntegralHeight)
			Cp.Style |= CBS_NOINTEGRALHEIGHT;

		switch (_FlatStyle)
		{
		case FlatStyle::Popup:
		case FlatStyle::Flat:
			Cp.ExStyle |= WS_EX_STATICEDGE;
		default:
			Cp.ExStyle |= WS_EX_CLIENTEDGE;
			break;
		}

		switch (_DropDownStyle)
		{
		case ComboBoxStyle::Simple:
			Cp.Style |= CBS_SIMPLE;
			break;
		case ComboBoxStyle::DropDown:
			Cp.Style |= CBS_DROPDOWN;
			break;
		case ComboBoxStyle::DropDownList:
			Cp.Style |= CBS_DROPDOWNLIST;
			break;
		}

		switch (_DrawMode)
		{
		case DrawMode::OwnerDrawFixed:
			Cp.Style |= CBS_OWNERDRAWFIXED;
			break;
		case DrawMode::OwnerDrawVariable:
			Cp.Style |= CBS_OWNERDRAWVARIABLE;
			break;
		}

		return Cp;
	}

	uintptr_t ComboBox::InitializeDCForWmCtlColor(HDC Dc, int32_t Message)
	{
		if ((Message == WM_CTLCOLORSTATIC))
		{
			return (uintptr_t)0;
		}
		else if ((Message == WM_CTLCOLORLISTBOX) && GetStyle(ControlStyles::UserPaint))
		{
			SetTextColor(Dc, this->ForeColor().ToCOLORREF());
			SetBkColor(Dc, this->BackColor().ToCOLORREF());

			return BackColorBrush();
		}
		else
		{
			return Control::InitializeDCForWmCtlColor(Dc, Message);
		}
	}

	int32_t ComboBox::NativeAdd(const char* Value)
	{
		return (int32_t)SendMessageA(this->_Handle, CB_ADDSTRING, NULL, (LPARAM)Value);
	}

	void ComboBox::NativeClear()
	{
		String Saved;

		if (_DropDownStyle != ComboBoxStyle::DropDownList)
			Saved = this->WindowText();

		SendMessageA(this->_Handle, CB_RESETCONTENT, NULL, NULL);

		if (!String::IsNullOrEmpty(Saved))
			this->SetWindowText(Saved);
	}

	int32_t ComboBox::NativeInsert(int32_t Index, const char* Value)
	{
		return (int32_t)SendMessageA(this->_Handle, CB_INSERTSTRING, (WPARAM)Index, (LPARAM)Value);
	}

	void ComboBox::NativeRemoveAt(int32_t Index)
	{
		if (_DropDownStyle == ComboBoxStyle::DropDownList && SelectedIndex() == Index)
			Invalidate();

		SendMessageA(this->_Handle, CB_DELETESTRING, (WPARAM)Index, NULL);
	}

	void ComboBox::WmReflectCommand(Message& Msg)
	{
		switch ((int16_t)HIWORD(Msg.WParam))
		{
		case CBN_EDITCHANGE:
			OnTextChanged();
			break;
		case CBN_SELCHANGE:
			OnSelectedIndexChanged();
			OnSelectedItemChanged();
			break;
		case CBN_DROPDOWN:
			OnDropDownOpened();
		case CBN_CLOSEUP:
			OnDropDownClosed();
			break;
		}
	}

	ComboBox::ComboBoxItemCollection::ComboBoxItemCollection(ComboBox* Owner)
		: _Owner(Owner), _Items()
	{
	}

	void ComboBox::ComboBoxItemCollection::Add(const imstring& Value)
	{
		_Items.EmplaceBack(Value);

		if (_Owner->GetState(ControlStates::StateCreated))
			_Owner->NativeAdd(Value);
	}

	void ComboBox::ComboBoxItemCollection::Insert(int32_t Index, const imstring& Value)
	{
		_Items.Insert(Index, Value);

		if (_Owner->GetState(ControlStates::StateCreated))
			_Owner->NativeInsert(Index, Value);
	}

	void ComboBox::ComboBoxItemCollection::Clear()
	{
		_Items.Clear();

		if (_Owner->GetState(ControlStates::StateCreated))
			_Owner->NativeClear();
	}

	bool ComboBox::ComboBoxItemCollection::Contains(const imstring & Value)
	{
		return (IndexOf(Value) > -1);
	}

	int32_t ComboBox::ComboBoxItemCollection::IndexOf(const imstring& Value)
	{
		auto Str = String(Value);
		auto Res = _Items.IndexOf(Str);

		if (Res == List<String>::InvalidPosition)
			return -1;

		return Res;
	}

	void ComboBox::ComboBoxItemCollection::RemoveAt(int32_t Index)
	{
		if (_Owner->GetState(ControlStates::StateCreated))
			_Owner->NativeRemoveAt(Index);

		_Items.RemoveAt(Index);
	}

	void ComboBox::ComboBoxItemCollection::Remove(const imstring& Value)
	{
		auto Index = IndexOf(Value);

		if (Index > -1)
			RemoveAt(Index);
	}

	uint32_t ComboBox::ComboBoxItemCollection::Count()
	{
		return _Items.Count();
	}

	String* ComboBox::ComboBoxItemCollection::begin() const
	{
		return _Items.begin();
	}

	String* ComboBox::ComboBoxItemCollection::end() const
	{
		return _Items.end();
	}
}
