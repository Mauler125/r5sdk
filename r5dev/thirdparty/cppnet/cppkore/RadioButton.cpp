#include "stdafx.h"
#include "RadioButton.h"

namespace Forms
{
	RadioButton::RadioButton()
		: ButtonBase(), _AutoCheck(true), _Appearence(Appearence::Normal), _Checked(false)
	{
		SetStyle(ControlStyles::StandardClick, false);
		this->SetTextAlign(Drawing::ContentAlignment::MiddleLeft);

		// We are a radiobutton control
		this->_RTTI = ControlTypes::RadioButton;
	}

	Appearence RadioButton::GetAppearence()
	{
		return this->_Appearence;
	}

	void RadioButton::SetAppearence(Appearence Value)
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

	bool RadioButton::AutoCheck()
	{
		return this->_AutoCheck;
	}

	void RadioButton::SetAutoCheck(bool Value)
	{
		this->_AutoCheck = Value;
	}

	bool RadioButton::Checked()
	{
		return this->_Checked;
	}

	void RadioButton::SetChecked(bool Value)
	{
		if (this->_Checked != Value)
		{
			this->_Checked = Value;

			if (GetState(ControlStates::StateCreated))
				SendMessageA(this->_Handle, BM_SETCHECK, Value ? 1 : 0, 0);

			Invalidate();
			Update();

			OnCheckedChanged();

			if (!this->_Parent)
				return;

			auto& ParentControls = this->_Parent->Controls();

			if (Value && ParentControls)
			{
				// Store the counts on the stack
				uint32_t ControlCount = ParentControls->Count();
				auto& ControlList = *ParentControls.get();

				for (uint32_t i = 0; i < ControlCount; i++)
				{
					auto Child = ControlList[i];
					auto ChildHwnd = Child->GetHandle();

					if (ChildHwnd == nullptr || Child == this)
						continue;

					if (Child->GetType() == ControlTypes::RadioButton)
						((RadioButton*)Child)->SetChecked(false);
				}
			}
		}
	}

	void RadioButton::OnClick()
	{
		if (this->_AutoCheck)
			this->SetChecked(true);

		// Call base event last
		ButtonBase::OnClick();
	}

	void RadioButton::OnAppearenceChanged()
	{
		AppearenceChanged.RaiseEvent(this);
	}

	void RadioButton::OnCheckedChanged()
	{
		CheckedChanged.RaiseEvent(this);
	}

	void RadioButton::OnHandleCreated()
	{
		SendMessageA(this->_Handle, BM_SETCHECK, _Checked ? 1 : 0, NULL);

		// We must call the base event last
		Control::OnHandleCreated();
	}

	void RadioButton::OnMouseUp(const std::unique_ptr<MouseEventArgs>& EventArgs)
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

					OnClick();
					OnMouseClick(EventArgs);
				}
			}
		}

		// Call base event last
		ButtonBase::OnMouseUp(EventArgs);
	}

	CreateParams RadioButton::GetCreateParams()
	{
		auto Cp = ButtonBase::GetCreateParams();

		Cp.ClassName = "BUTTON";

		if (this->_OwnerDraw)
			Cp.Style |= BS_OWNERDRAW;
		else
		{
			Cp.Style |= BS_RADIOBUTTON;

			if (this->_Appearence == Appearence::Button)
				Cp.Style |= BS_PUSHLIKE;
		}

		return Cp;
	}
}