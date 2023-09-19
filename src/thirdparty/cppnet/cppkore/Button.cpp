#include "stdafx.h"
#include "Button.h"
#include "Form.h"

namespace Forms
{
	Button::Button()
		: ButtonBase(), _DialogResult(DialogResult::None)
	{
		SetStyle(ControlStyles::StandardClick | ControlStyles::StandardDoubleClick, false);

		// We are a button control
		this->_RTTI = ControlTypes::Button;
	}

	DialogResult Button::GetDialogResult()
	{
		return this->_DialogResult;
	}

	void Button::SetDialogResult(DialogResult Value)
	{
		this->_DialogResult = Value;
	}

	void Button::PerformClick()
	{
		if (CanSelect())
		{
			ResetFlagsAndPaint();
			OnClick();
		}
	}

	void Button::NotifyDefault(bool Value)
	{
		if (IsDefault() != Value)
			SetIsDefault(Value);
	}

	void Button::OnClick()
	{
		auto Form = this->FindForm();

		if (Form != nullptr)
			((Forms::Form*)Form)->SetDialogResult(this->_DialogResult);

		// Call base event last
		ButtonBase::OnClick();
	}

	void Button::OnMouseUp(const std::unique_ptr<MouseEventArgs>& EventArgs)
	{
		if (EventArgs->Button == MouseButtons::Left && GetFlag(ButtonFlags::FlagMousePressed))
		{
			auto isMouseDown = GetFlag(ButtonFlags::FlagMouseDown);

			if (GetStyle(ControlStyles::UserPaint))
			{
				this->ResetFlagsAndPaint();
			}

			if (isMouseDown)
			{
				auto Pt = PointToScreen({ (INT)EventArgs->X, (INT)EventArgs->Y });

				POINT nPt;
				nPt.x = Pt.X;
				nPt.y = Pt.Y;

				if (WindowFromPoint(nPt) == this->_Handle)
				{
					if (GetStyle(ControlStyles::UserPaint))
						OnClick();

					OnMouseClick(EventArgs);
				}
			}
		}

		// Call base event last
		ButtonBase::OnMouseUp(EventArgs);
	}

	CreateParams Button::GetCreateParams()
	{
		auto Cp = ButtonBase::GetCreateParams();

		Cp.ClassName = "BUTTON";

		if (GetStyle(ControlStyles::UserPaint))
			Cp.Style |= BS_OWNERDRAW;
		else
		{
			Cp.Style |= BS_MULTILINE;
			Cp.Style |= BS_PUSHBUTTON;

			if (this->IsDefault())
				Cp.Style |= BS_DEFPUSHBUTTON;
		}

		return Cp;
	}
}
