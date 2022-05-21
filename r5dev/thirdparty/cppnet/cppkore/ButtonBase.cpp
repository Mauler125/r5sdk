#include "stdafx.h"
#include "ButtonBase.h"

namespace Forms
{
	ButtonBase::ButtonBase()
		: Control(), _OwnerDraw(false), _Flags((ButtonFlags)0), _FlatStyle(FlatStyle::Standard), _TextAlign(Drawing::ContentAlignment::MiddleCenter)
	{
		SetStyle(ControlStyles::SupportsTransparentBackColor |
			ControlStyles::Opaque |
			ControlStyles::ResizeRedraw |
			ControlStyles::OptimizedDoubleBuffer |
			ControlStyles::CacheText |
			ControlStyles::StandardClick, true);

		SetStyle(ControlStyles::UserMouse |
			ControlStyles::UserPaint, this->_OwnerDraw);
	}

	bool ButtonBase::OwnerDraw()
	{
		return this->_OwnerDraw;
	}

	void ButtonBase::SetOwnerDraw(bool Value)
	{
		this->_OwnerDraw = Value;

		SetStyle(ControlStyles::UserMouse | ControlStyles::UserPaint, Value);

		UpdateStyles();
		Invalidate();
	}

	Drawing::ContentAlignment ButtonBase::TextAlign()
	{
		return this->_TextAlign;
	}

	void ButtonBase::SetTextAlign(Drawing::ContentAlignment Value)
	{
		this->_TextAlign = Value;

		if (this->_OwnerDraw)
			Invalidate();
		else
			UpdateStyles();
	}

	FlatStyle ButtonBase::GetFlatStyle()
	{
		return this->_FlatStyle;
	}

	void ButtonBase::SetFlatStyle(FlatStyle Value)
	{
		this->_FlatStyle = Value;

		Invalidate();

		// Force update styles...
		SetStyle(ControlStyles::UserMouse | ControlStyles::UserPaint, this->_OwnerDraw);
		UpdateStyles();
	}

	bool ButtonBase::IsDefault()
	{
		return GetFlag(ButtonFlags::FlagIsDefault);
	}

	void ButtonBase::SetIsDefault(bool Value)
	{
		if (IsDefault() != Value)
		{
			SetFlag(ButtonFlags::FlagIsDefault, Value);

			if (this->_OwnerDraw)
				Invalidate();
			else
				UpdateStyles();
		}
	}

	bool ButtonBase::GetFlag(ButtonFlags Flag)
	{
		return ((int)this->_Flags & (int)Flag) == (int)Flag;
	}

	void ButtonBase::SetFlag(ButtonFlags Flags, bool Value)
	{
		this->_Flags = Value ? (ButtonFlags)((int)this->_Flags | (int)Flags) : (ButtonFlags)((int)this->_Flags & ~(int)Flags);
	}

	void ButtonBase::OnLostFocus()
	{
		Control::OnLostFocus();

		// Hitting tab while holding down the space key
		SetFlag(ButtonFlags::FlagMouseDown, false);
		SetCapture(false);

		Invalidate();
	}

	void ButtonBase::OnGotFocus()
	{
		Control::OnGotFocus();
		Invalidate();
	}

	void ButtonBase::OnMouseEnter()
	{
		SetFlag(ButtonFlags::FlagMouseOver, true);
		Invalidate();

		// Call base event last
		Control::OnMouseEnter();
	}

	void ButtonBase::OnMouseLeave()
	{
		SetFlag(ButtonFlags::FlagMouseOver, false);
		Invalidate();

		// Call base event last
		Control::OnMouseLeave();
	}

	void ButtonBase::OnEnabledChanged()
	{
		Control::OnEnabledChanged();

		if (!Enabled())
		{
			SetFlag(ButtonFlags::FlagMouseDown | ButtonFlags::FlagMouseOver, false);
			Invalidate();
		}
	}

	void ButtonBase::OnTextChanged()
	{
		Control::OnTextChanged();
		Invalidate();
	}

	void ButtonBase::OnMouseMove(const std::unique_ptr<MouseEventArgs>& EventArgs)
	{
		if (EventArgs->Button != MouseButtons::None && GetFlag(ButtonFlags::FlagMousePressed))
		{
			auto CRect = this->ClientRectangle();
			
			if (!CRect.Contains(EventArgs->X, EventArgs->Y))
			{
				if (GetFlag(ButtonFlags::FlagMouseDown))
				{
					SetFlag(ButtonFlags::FlagMouseDown, false);
					Invalidate();
				}
			}
			else
			{
				if (!GetFlag(ButtonFlags::FlagMouseDown))
				{
					SetFlag(ButtonFlags::FlagMouseDown, true);
					Invalidate();
				}
			}
		}

		// Call base event last
		Control::OnMouseMove(EventArgs);
	}

	void ButtonBase::OnMouseDown(const std::unique_ptr<MouseEventArgs>& EventArgs)
	{
		if (EventArgs->Button == MouseButtons::Left)
		{
			SetFlag(ButtonFlags::FlagMouseDown | ButtonFlags::FlagMousePressed, true);
			Invalidate();
		}

		// Call base event last
		Control::OnMouseDown(EventArgs);
	}

	void ButtonBase::OnMouseUp(const std::unique_ptr<MouseEventArgs>& EventArgs)
	{
		// Call base event last
		Control::OnMouseUp(EventArgs);
	}

	void ButtonBase::OnKeyUp(const std::unique_ptr<KeyEventArgs>& EventArgs)
	{
		if (GetFlag(ButtonFlags::FlagMouseDown))
		{
			if (this->_OwnerDraw)
				ResetFlagsAndPaint();
			else
			{
				SetFlag(ButtonFlags::FlagMousePressed | ButtonFlags::FlagMouseDown, false);
				SendMessageA(this->_Handle, BM_SETSTATE, 0, 0);
			}

			if (EventArgs->KeyCode() == Keys::Enter || EventArgs->KeyCode() == Keys::Space)
				OnClick();

			EventArgs->SetHandled(true);
		}

		// Call base event last
		Control::OnKeyUp(EventArgs);
	}

	void ButtonBase::OnKeyDown(const std::unique_ptr<KeyEventArgs>& EventArgs)
	{
		if (EventArgs->KeyData() == Keys::Space)
		{
			if (!GetFlag(ButtonFlags::FlagMouseDown))
			{
				SetFlag(ButtonFlags::FlagMouseDown, true);

				if (!this->_OwnerDraw)
					SendMessageA(this->_Handle, BM_SETSTATE, 1, 0);

				Invalidate();
			}

			EventArgs->SetHandled(true);
		}

		// Call base event last
		Control::OnKeyDown(EventArgs);
	}

	void ButtonBase::WndProc(Message& Msg)
	{
		switch (Msg.Msg)
		{
		case BM_CLICK:
			OnClick();
			return;
		}

		if (this->_OwnerDraw)
		{
			switch (Msg.Msg)
			{
			case BM_SETSTATE:
				break;

			case WM_KILLFOCUS:
			case WM_CANCELMODE:
			case WM_CAPTURECHANGED:

				if (!GetFlag(ButtonFlags::FlagInButtonUp) && GetFlag(ButtonFlags::FlagMousePressed)) 
				{
					SetFlag(ButtonFlags::FlagMousePressed, false);

					if (GetFlag(ButtonFlags::FlagMouseDown))
					{
						SetFlag(ButtonFlags::FlagMouseDown, false);
						Invalidate();
					}
				}

				Control::WndProc(Msg);
				break;

			case WM_LBUTTONUP:
			case WM_MBUTTONUP:
			case WM_RBUTTONUP:

				SetFlag(ButtonFlags::FlagInButtonUp, true);
				Control::WndProc(Msg);
				SetFlag(ButtonFlags::FlagInButtonUp, false);

			break;

			default:
				Control::WndProc(Msg);
				break;
			}
		}
		else
		{
			if (Msg.Msg == (WM_REFLECT + WM_COMMAND) && HIWORD(Msg.WParam) == BN_CLICKED)
				OnClick();
			else
				Control::WndProc(Msg);
		}
	}

	CreateParams ButtonBase::GetCreateParams()
	{
		auto Cp = Control::GetCreateParams();

		if (!this->_OwnerDraw)
		{
			Cp.ExStyle &= ~WS_EX_RIGHT;	// Messes up the BM_ Alignment flags

			Cp.Style |= BS_MULTILINE;

			if (IsDefault())
				Cp.Style |= BS_DEFPUSHBUTTON;

			if (((int)this->_TextAlign & (int)Drawing::AnyLeftAlign) != 0)
				Cp.Style |= BS_LEFT;
			else if (((int)this->_TextAlign & (int)Drawing::AnyRightAlign) != 0)
				Cp.Style |= BS_RIGHT;
			else
				Cp.Style |= BS_CENTER;

			if (((int)this->_TextAlign & (int)Drawing::AnyTopAlign) != 0)
				Cp.Style |= BS_TOP;
			else if (((int)this->_TextAlign & (int)Drawing::AnyBottomAlign) != 0)
				Cp.Style |= BS_BOTTOM;
			else
				Cp.Style |= BS_VCENTER;
		}

		return Cp;
	}

	void ButtonBase::ResetFlagsAndPaint()
	{
		SetFlag(ButtonFlags::FlagMousePressed | ButtonFlags::FlagMouseDown, false);
		Invalidate();
		Update();
	}
}
