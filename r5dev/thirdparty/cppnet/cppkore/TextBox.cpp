#include "stdafx.h"
#include "TextBox.h"

namespace Forms
{
	TextBox::TextBox()
		: TextBoxBase(), _AcceptsReturn(false), _PasswordChar('\0'), _UseSystemPasswordChar(false), _CharacterCasing(CharacterCasing::Normal), _Scrollbars(ScrollBars::None), _TextAlign(HorizontalAlignment::Left), _SelectionSet(false), _SizeRectDirty(true), _IsCalcRects(false), _NCRectTop{}, _NCRectBottom{}, _NCRectLeft{}, _NCRectRight{}
	{
		// We are a textbox control
		this->_RTTI = ControlTypes::TextBox;
	}

	bool TextBox::AcceptsReturn()
	{
		return this->_AcceptsReturn;
	}

	void TextBox::AcceptsReturn(bool Value)
	{
		this->_AcceptsReturn = Value;
	}

	CharacterCasing TextBox::GetCharacterCasing()
	{
		return this->_CharacterCasing;
	}

	void TextBox::SetCharacterCasing(CharacterCasing Value)
	{
		if (_CharacterCasing != Value)
		{
			_CharacterCasing = Value;
			UpdateStyles();
		}
	}

	bool TextBox::PasswordProtect()
	{
		return (_PasswordChar != '\0');
	}

	char TextBox::PasswordChar()
	{
		if (GetState(ControlStates::StateCreated))
			return (char)SendMessageA(this->_Handle, EM_GETPASSWORDCHAR, NULL, NULL);
		
		return this->_PasswordChar;
	}

	void TextBox::SetPasswordChar(char Value)
	{
		_PasswordChar = Value;

		if (!_UseSystemPasswordChar && GetState(ControlStates::StateCreated) && PasswordChar() != Value)
		{
			// Set the password mode
			SendMessageA(this->_Handle, EM_SETPASSWORDCHAR, Value, NULL);
			Invalidate();
		}
	}

	ScrollBars TextBox::GetScrollBars()
	{
		return this->_Scrollbars;
	}

	void TextBox::SetScrollBars(ScrollBars Value)
	{
		if (_Scrollbars != Value)
		{
			_Scrollbars = Value;
			UpdateStyles();
		}
	}

	void TextBox::SetText(const string& Value)
	{
		TextBoxBase::SetText(Value);
		_SelectionSet = false;
	}

	HorizontalAlignment TextBox::TextAlign()
	{
		return this->_TextAlign;
	}

	void TextBox::SetTextAlign(HorizontalAlignment Value)
	{
		if (_TextAlign != Value)
		{
			_TextAlign = Value;
			UpdateStyles();
		}
	}

	bool TextBox::UseSystemPasswordChar()
	{
		return this->_UseSystemPasswordChar;
	}

	void TextBox::SetUseSystemPasswordChar(bool Value)
	{
		if (_UseSystemPasswordChar != Value)
		{
			_UseSystemPasswordChar = Value;
			UpdateStyles();
		}
	}

	void TextBox::OnSizeChanged()
	{
		if (!GetFlag(TextBoxFlags::FlagMultiline) && !_IsCalcRects)
		{
			_SizeRectDirty = true;
			CalculateSizeRects();
		}

		// We must call the base event last
		TextBoxBase::OnSizeChanged();
	}

	void TextBox::OnHandleCreated()
	{
		TextBoxBase::OnHandleCreated();

		if (_PasswordChar != '\0' && !_UseSystemPasswordChar)
			SendMessageA(this->_Handle, EM_SETPASSWORDCHAR, _PasswordChar, NULL);
	}

	void TextBox::OnGotFocus()
	{
		TextBoxBase::OnGotFocus();

		if (!_SelectionSet)
		{
			// We get one shot at selecting when we first get focus.  If we don't
			// do it, we still want to act like the selection was set.
			_SelectionSet = true;

			// If the user didn't provide a selection, force one in.
			if (SelectionLength() == 0 && Control::GetMouseButtons() == MouseButtons::None)
				SelectAll();
		}
	}

	void TextBox::WndProc(Message& Msg)
	{
		switch (Msg.Msg)
		{
		case WM_NCPAINT:
			WmNcPaint(Msg);
			break;
		case WM_NCCALCSIZE:
			if (!GetFlag(TextBoxFlags::FlagMultiline))
				WmNcCalcSize(Msg);
			else
				TextBoxBase::WndProc(Msg);
			break;
		default:
			TextBoxBase::WndProc(Msg);
			break;
		}
	}

	CreateParams TextBox::GetCreateParams()
	{
		auto Cp = TextBoxBase::GetCreateParams();

		switch (_CharacterCasing)
		{
		case CharacterCasing::Lower:
			Cp.Style |= ES_LOWERCASE;
			break;
		case CharacterCasing::Upper:
			Cp.Style |= ES_UPPERCASE;
			break;
		}

		Cp.ExStyle &= ~WS_EX_RIGHT;
		switch (_TextAlign)
		{
		case HorizontalAlignment::Left:
			Cp.Style |= ES_LEFT;
			break;
		case HorizontalAlignment::Center:
			Cp.Style |= ES_CENTER;
			break;
		case HorizontalAlignment::Right:
			Cp.Style |= ES_RIGHT;
			break;
		}

		if (Multiline())
		{
			if (((int)_Scrollbars & (int)ScrollBars::Horizontal) == (int)ScrollBars::Horizontal && _TextAlign == HorizontalAlignment::Left && !WordWrap())
				Cp.Style |= WS_HSCROLL;

			if (((int)_Scrollbars & (int)ScrollBars::Vertical) == (int)ScrollBars::Vertical)
				Cp.Style |= WS_VSCROLL;
		}

		if (_UseSystemPasswordChar)
			Cp.Style |= ES_PASSWORD;

		return Cp;
	}

	void TextBox::WmNcCalcSize(Message& Msg)
	{
		if (Msg.WParam == 0)
		{
			TextBoxBase::WndProc(Msg);
			return;
		}

		RECT RectWnd, RectClient, RectText{};
		GetClientRect(this->_Handle, &RectClient);
		GetWindowRect(this->_Handle, &RectWnd);

		auto Font = this->GetFont();
		auto DC = GetDC(this->_Handle);

		auto pOld = SelectObject(DC, Font->GetFontHandle());
		DrawTextA(DC, "Ky", 2, &RectText, DT_CALCRECT | DT_LEFT);
		SelectObject(DC, pOld);

		ReleaseDC(this->_Handle, DC);
		MapWindowPoints(this->_Handle, NULL, (LPPOINT)&RectClient, 2);

		UINT uiVClientHeight = RectText.bottom - RectText.top;

		auto lpNCSP = (NCCALCSIZE_PARAMS FAR*)Msg.LParam;

		UINT uiCenterOffset = ((RectClient.bottom - RectClient.top) - uiVClientHeight) / 2;
		UINT uiCY = ((RectWnd.bottom - RectWnd.top) - (RectClient.bottom - RectClient.top)) / 2;
		UINT uiCX = ((RectWnd.right - RectWnd.left) - (RectClient.right - RectClient.left)) / 2;

		// Handle special case where RectClient == RectWnd
		if (this->GetBorderStyle() == BorderStyle::None)
		{
			uiCY = 3;
			uiCX = 3;
			uiCenterOffset -= 1;
		}

		// A shift to adjust for the off by 1 pixel
		uiCenterOffset += 1;

		OffsetRect(&RectWnd, -RectWnd.left, -RectWnd.top);

		// Handle special case where RectClient == RectWnd
		if (this->GetBorderStyle() == BorderStyle::None)
		{
			_NCRectTop.top = 1;
			_NCRectTop.left = 1;
			_NCRectTop.right = RectWnd.right - 1;
			_NCRectTop.bottom = uiCenterOffset;

			_NCRectBottom.left = 1;
			_NCRectBottom.top = uiCenterOffset + uiVClientHeight;
			_NCRectBottom.right = RectWnd.right - 1;
			_NCRectBottom.bottom = RectWnd.bottom - 1;

			_NCRectLeft = _NCRectTop;
			_NCRectLeft.right = _NCRectLeft.left + 2;
			_NCRectLeft.bottom = _NCRectBottom.bottom;

			_NCRectRight = _NCRectTop;
			_NCRectRight.left = (_NCRectRight.right - 2);
			_NCRectRight.bottom = _NCRectBottom.bottom;
		}
		else
		{
			_NCRectTop = RectWnd;

			_NCRectTop.left += uiCX;
			_NCRectTop.top += uiCY;
			_NCRectTop.right -= uiCX;
			_NCRectTop.bottom -= (uiCenterOffset + uiVClientHeight + uiCY);

			_NCRectBottom = RectWnd;

			_NCRectBottom.left += uiCX;
			_NCRectBottom.top += (uiCenterOffset + uiVClientHeight + uiCY);
			_NCRectBottom.right -= uiCX;
			_NCRectBottom.bottom -= uiCY;

			_NCRectLeft = _NCRectTop;
			_NCRectLeft.left -= 1;
			_NCRectLeft.right = _NCRectLeft.left + 1;
			_NCRectLeft.bottom = _NCRectBottom.bottom;

			_NCRectRight = _NCRectTop;
			_NCRectRight.left = (_NCRectRight.right - 1);
			_NCRectRight.right += 1;
			_NCRectRight.bottom = _NCRectBottom.bottom;
		}

		// Set the resulting client size
		lpNCSP->rgrc[0].top += uiCenterOffset;
		lpNCSP->rgrc[0].bottom -= uiCenterOffset;
		lpNCSP->rgrc[0].left += uiCX;
		lpNCSP->rgrc[0].right -= uiCY;

		// They aren't dirty after this routine
		_SizeRectDirty = false;
	}

	void TextBox::WmNcPaint(Message& Msg)
	{
		if (!GetFlag(TextBoxFlags::FlagMultiline))
		{
			// Check if we need to recalculate the size rects
			if (_SizeRectDirty)
				CalculateSizeRects();

			// Paint the missing rects
			auto DC = GetWindowDC(this->_Handle);
			auto Brush = (HBRUSH)this->BackColorBrush();

			FillRect(DC, &_NCRectLeft, Brush);
			FillRect(DC, &_NCRectRight, Brush);
			FillRect(DC, &_NCRectTop, Brush);
			FillRect(DC, &_NCRectBottom, Brush);
			
			ReleaseDC(this->_Handle, DC);
		}

		// Ensure default action is taken, WmNcPaint won't touch outside the NC rect...
		TextBoxBase::WndProc(Msg);
	}

	void TextBox::CalculateSizeRects()
	{
		_IsCalcRects = true;
		SetWindowPos(this->_Handle, NULL, 0, 0, 0, 0, SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOMOVE | SWP_FRAMECHANGED);
		_IsCalcRects = false;
	}
}
