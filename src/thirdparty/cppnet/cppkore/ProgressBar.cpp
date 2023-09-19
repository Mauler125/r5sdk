#include "stdafx.h"
#include "ProgressBar.h"

namespace Forms
{
	ProgressBar::ProgressBar()
		: Control(), _Minimum(0), _Maximum(100), _Step(10), _Value(0), _MarqueeSpeed(100)
	{
		SetStyle(ControlStyles::UserPaint |
			ControlStyles::UseTextForAccessibility |
			ControlStyles::Selectable, false);
	}

	void ProgressBar::CreateControl(Control* Parent)
	{
		INITCOMMONCONTROLSEX iCC{};
		iCC.dwSize = sizeof(iCC);
		iCC.dwICC = ICC_PROGRESS_CLASS;

		InitCommonControlsEx(&iCC);

		Control::CreateControl(Parent);
	}

	void ProgressBar::OnHandleCreated()
	{
		// Range position and step
		SendMessageA(this->_Handle, PBM_SETRANGE32, _Minimum, _Maximum);
		SendMessageA(this->_Handle, PBM_SETSTEP, _Step, NULL);
		SendMessageA(this->_Handle, PBM_SETPOS, _Value, NULL);
		// Colors
		SendMessageA(this->_Handle, PBM_SETBKCOLOR, NULL, Drawing::ColorToWin32(this->_BackColor));
		SendMessageA(this->_Handle, PBM_SETBARCOLOR, NULL, Drawing::ColorToWin32(this->_ForeColor));
		// Animation
		StartMarquee();

		// We must call the base event last
		Control::OnHandleCreated();
	}

	void ProgressBar::OnBackColorChanged()
	{
		Control::OnBackColorChanged();

		if (GetState(ControlStates::StateCreated))
			SendMessageA(this->_Handle, PBM_SETBKCOLOR, NULL, Drawing::ColorToWin32(this->_BackColor));
	}

	void ProgressBar::OnForeColorChanged()
	{
		Control::OnForeColorChanged();

		if (GetState(ControlStates::StateCreated))
			SendMessageA(this->_Handle, PBM_SETBARCOLOR, NULL, Drawing::ColorToWin32(this->_ForeColor));
	}

	uint32_t ProgressBar::Value()
	{
		return this->_Value;
	}

	void ProgressBar::SetValue(uint32_t Value)
	{
		if (Value > this->_Maximum)
			Value = this->_Maximum;
		if (Value < this->_Minimum)
			Value = this->_Minimum;

		this->_Value = Value;

		UpdatePos();
	}

	uint32_t ProgressBar::MarqueeAnimationSpeed()
	{
		return this->_MarqueeSpeed;
	}

	void ProgressBar::SetMarqueeAnimationSpeed(uint32_t Value)
	{
		this->_MarqueeSpeed = Value;
		StartMarquee();
	}

	uint32_t ProgressBar::Maximum()
	{
		return this->_Maximum;
	}

	void ProgressBar::SetMaximum(uint32_t Value)
	{
		if (this->_Maximum != Value)
		{
			if (this->_Minimum > Value)
				this->_Minimum = Value;

			this->_Maximum = Value;

			if (this->_Value > Value)
				this->_Value = this->_Value;

			if (GetState(ControlStates::StateCreated))
			{
				SendMessageA(this->_Handle, PBM_SETRANGE32, this->_Minimum, this->_Maximum);
				UpdatePos();
			}
		}
	}

	uint32_t ProgressBar::Minimum()
	{
		return this->_Minimum;
	}

	void ProgressBar::SetMinimum(uint32_t Value)
	{
		if (this->_Minimum != Value)
		{
			if (this->_Maximum < Value)
				this->_Maximum = Value;

			this->_Minimum = Value;

			if (this->_Value < Value)
				this->_Value = Value;

			if (GetState(ControlStates::StateCreated))
			{
				SendMessageA(this->_Handle, PBM_SETRANGE32, this->_Minimum, this->_Maximum);
				UpdatePos();
			}
		}
	}

	uint32_t ProgressBar::Step()
	{
		return this->_Step;
	}

	void ProgressBar::SetStep(uint32_t Value)
	{
		this->_Step = Value;

		if (GetState(ControlStates::StateCreated))
			SendMessageA(this->_Handle, PBM_SETSTEP, this->_Step, NULL);
	}

	ProgressBarStyle ProgressBar::ProgressStyle()
	{
		return this->_Style;
	}

	void ProgressBar::SetProgressStyle(ProgressBarStyle Value)
	{
		if (this->_Style != Value)
		{
			this->_Style = Value;

			// Redraw for ownerdraw components, and force update our styles
			Invalidate();
			UpdateStyles();
		}
	}

	void ProgressBar::Increment(uint32_t Value)
	{
		this->_Value = Value;

		if (this->_Value > this->_Maximum)
			this->_Value = this->_Maximum;
		if (this->_Value < this->_Minimum)
			this->_Value = this->_Minimum;

		UpdatePos();
	}

	void ProgressBar::PerformStep()
	{
		Increment(this->_Step);
	}

	CreateParams ProgressBar::GetCreateParams()
	{
		auto Cp = Control::GetCreateParams();
		Cp.ClassName = "msctls_progress32";

		if (this->_Style == ProgressBarStyle::Continuous)
			Cp.Style |= PBS_SMOOTH;
		else if (this->_Style == ProgressBarStyle::Marquee)
			Cp.Style |= PBS_MARQUEE;

		return Cp;
	}

	void ProgressBar::StartMarquee()
	{
		if (GetState(ControlStates::StateCreated) && this->_Style == ProgressBarStyle::Marquee)
		{
			if (this->_MarqueeSpeed == 0)
				SendMessageA(this->_Handle, PBM_SETMARQUEE, 0, _MarqueeSpeed);
			else
				SendMessageA(this->_Handle, PBM_SETMARQUEE, 1, _MarqueeSpeed);
		}
	}

	void ProgressBar::UpdatePos()
	{
		if (GetState(ControlStates::StateCreated))
			SendMessageA(this->_Handle, PBM_SETPOS, _Value, NULL);
	}
}