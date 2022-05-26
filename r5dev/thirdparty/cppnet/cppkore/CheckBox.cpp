#include "stdafx.h"
#include "CheckBox.h"

namespace Forms
{
	CheckBox::CheckBox()
		: ButtonBase(), _AutoCheck(true), _ThreeState(false), _Appearence(Appearence::Normal), _CheckState(CheckState::Unchecked)
	{
		SetStyle(ControlStyles::StandardClick | ControlStyles::StandardDoubleClick, false);
		this->SetTextAlign(Drawing::ContentAlignment::MiddleLeft);

		// We are a checkbox control
		this->_RTTI = ControlTypes::CheckBox;
	}

	Appearence CheckBox::GetAppearence()
	{
		return this->_Appearence;
	}

	void CheckBox::SetAppearence(Appearence Value)
	{
		if (this->_Appearence != Value)
		{
			this->_Appearence = Value;

			if (this->_OwnerDraw)
				Refresh();
			else
				UpdateStyles();

			OnAppearenceChanged();
		}
	}

	bool CheckBox::AutoCheck()
	{
		return this->_AutoCheck;
	}

	void CheckBox::SetAutoCheck(bool Value)
	{
		this->_AutoCheck = Value;
	}

	bool CheckBox::Checked()
	{
		return _CheckState != CheckState::Unchecked;
	}

	void CheckBox::SetChecked(bool Value)
	{
		this->SetCheckState((Value) ? CheckState::Checked : CheckState::Unchecked);
	}

	CheckState CheckBox::GetCheckState()
	{
		return _CheckState;
	}

	void CheckBox::SetCheckState(CheckState Value)
	{
		if (_CheckState != Value)
		{
			bool oChecked = Checked();

			_CheckState = Value;

			if (GetState(ControlStates::StateCreated))
				SendMessageA(this->_Handle, BM_SETCHECK, (int)_CheckState, 0);

			if (oChecked != Checked())
			{
				OnCheckedChanged();
			}
			
			OnCheckStateChanged();
		}
	}

	bool CheckBox::ThreeState()
	{
		return _ThreeState;
	}

	void CheckBox::SetThreeState(bool Value)
	{
		_ThreeState = Value;
	}

	void CheckBox::OnClick()
	{
		if (_AutoCheck)
		{
			switch (_CheckState)
			{
			case CheckState::Unchecked:
				SetCheckState(CheckState::Checked);
				break;
			case CheckState::Checked:
				if (_ThreeState)
					SetCheckState(CheckState::Indeterminate);
				else
					SetCheckState(CheckState::Unchecked);
				break;
			default:
				SetCheckState(CheckState::Unchecked);
				break;
			}
		}

		// Call base event last
		ButtonBase::OnClick();
	}

	void CheckBox::OnAppearenceChanged()
	{
		AppearenceChanged.RaiseEvent(this);
	}

	void CheckBox::OnCheckedChanged()
	{
		CheckedChanged.RaiseEvent(this);
	}

	void CheckBox::OnCheckStateChanged()
	{
		if (this->_OwnerDraw)
			Refresh();

		CheckStateChanged.RaiseEvent(this);
	}

	void CheckBox::OnHandleCreated()
	{
		SendMessageA(this->_Handle, BM_SETCHECK, (int)_CheckState, NULL);

		// We must call base event last
		Control::OnHandleCreated();
	}

	void CheckBox::OnMouseUp(const std::unique_ptr<MouseEventArgs>& EventArgs)
	{
		if (EventArgs->Button == MouseButtons::Left && GetFlag(ButtonFlags::FlagMousePressed))
		{
			if (GetFlag(ButtonFlags::FlagMouseDown))
			{
				auto Pt = PointToScreen({ (INT)EventArgs->X, (INT)EventArgs->Y });

				POINT nPt;
				nPt.x = Pt.X;
				nPt.y = Pt.Y;

				if (WindowFromPoint(nPt) == this->_Handle)
				{
					ResetFlagsAndPaint();

					if (this->Capture())
						OnClick();

					OnMouseClick(EventArgs);
				}
			}
		}

		// Call base event last
		ButtonBase::OnMouseUp(EventArgs);
	}

	CreateParams CheckBox::GetCreateParams()
	{
		auto Cp = ButtonBase::GetCreateParams();

		Cp.ClassName = "BUTTON";

		if (GetStyle(ControlStyles::UserPaint))
			Cp.Style |= BS_OWNERDRAW;
		else
		{
			Cp.Style |= BS_3STATE;
			
			if (this->_Appearence == Appearence::Button)
				Cp.Style |= BS_PUSHLIKE;
		}

		return Cp;
	}
}
