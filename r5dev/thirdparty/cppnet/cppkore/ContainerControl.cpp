#include "stdafx.h"
#include "ContainerControl.h"
#include "Form.h"

namespace Forms
{
	ContainerControl::ContainerControl()
		: Control(), _ActiveControl(nullptr), _FocusedControl(nullptr), _AutoScaleDimensions{}, _CurrentAutoScaleDimensions{}, _AutoScaleMode(AutoScaleMode::Inherit), _ScalingNeededOnLayout(false)
	{
		SetStyle(ControlStyles::AllPaintingInWmPaint, false);
	}

	Control* ContainerControl::ActiveControl()
	{
		return this->_ActiveControl;
	}

	void ContainerControl::SetActiveControl(Control* Value)
	{
		SetActiveControlInternal(Value);
	}

	bool ContainerControl::ActivateControl(Control* Value)
	{
		return ActivateControlInternal(Value, true);
	}

	Drawing::SizeF ContainerControl::AutoScaleDimensions()
	{
		return this->_AutoScaleDimensions;
	}

	void ContainerControl::SetAutoScaleDimensions(Drawing::SizeF Size)
	{
		this->_AutoScaleDimensions = Size;
		if (!this->_AutoScaleDimensions.Empty())
		{
			this->LayoutScalingNeeded();
		}
	}

	Drawing::SizeF ContainerControl::CurrentAutoScaleDimensions()
	{
		if (this->_CurrentAutoScaleDimensions.Empty())
		{
			switch (this->_AutoScaleMode)
			{
			case AutoScaleMode::Font:
				this->_CurrentAutoScaleDimensions = this->GetFontAutoScaleDimensions();
				break;
			case AutoScaleMode::Dpi:
				// TODO: Handle dpi related scaling values...
				break;

			default:
				this->_CurrentAutoScaleDimensions = this->AutoScaleDimensions();
				break;
			}
		}

		return this->_CurrentAutoScaleDimensions;
	}

	AutoScaleMode ContainerControl::AutoScaleMode()
	{
		return this->_AutoScaleMode;
	}

	void ContainerControl::SetAutoScaleMode(Forms::AutoScaleMode Mode)
	{
		bool ScalingRequired = false;

		if (Mode != this->_AutoScaleMode)
		{
			if (_AutoScaleMode != Forms::AutoScaleMode::Inherit)
			{
				this->_AutoScaleDimensions = Drawing::SizeF{};
			}

			this->_AutoScaleMode = Mode;
			ScalingRequired = true;
		}

		// TODO: OnAutoScaleModeChanged();

		if (ScalingRequired)
			this->LayoutScalingNeeded();
	}

	void ContainerControl::WndProc(Message& Msg)
	{
		switch (Msg.Msg)
		{
		case WM_SETFOCUS:
			WmSetFocus(Msg);
			break;
		default:
			Control::WndProc(Msg);
			break;
		}
	}

	CreateParams ContainerControl::GetCreateParams()
	{
		auto Cp = Control::GetCreateParams();
		Cp.ExStyle |= WS_EX_CONTROLPARENT;

		return Cp;
	}

	bool ContainerControl::IsContainerControl()
	{
		return true;
	}

	void ContainerControl::WmSetFocus(Message& Msg)
	{
		if (_ActiveControl != nullptr)
		{
			if (!_ActiveControl->Visible())
				this->OnGotFocus();

			FocusActiveControlInternal();
		}
		else
		{
			if (_Parent != nullptr)
			{
				ContainerControl* C = (ContainerControl*)_Parent->GetContainerControl();
				if (C != nullptr)
				{
					if (!C->ActivateControlInternal(this, true))
						return;
				}
			}

			Control::WndProc(Msg);
		}
	}

	void ContainerControl::LayoutScalingNeeded()
	{
		this->EnableRequiredScaling(this, true);
		this->_ScalingNeededOnLayout = true;
	}

	void ContainerControl::EnableRequiredScaling(Control *Ctrl, bool Enable)
	{
		Ctrl->SetRequiredScalingEnabled(Enable);

		if (!Ctrl->GetStyle(ControlStyles::ContainerControl) || Ctrl->Controls() == nullptr)
			return;

		uint32_t ControlCount = Ctrl->Controls()->Count();
		auto& ControlList = *Ctrl->Controls().get();

		for (uint32_t i = 0; i < ControlCount; i++)
		{
			EnableRequiredScaling(ControlList[i], Enable);
		}
	}

	void ContainerControl::PerformNeededAutoScaleOnLayout()
	{
		if (this->_ScalingNeededOnLayout)
		{
			this->PerformAutoScale(this->_ScalingNeededOnLayout, false);
		}
	}

	void ContainerControl::PerformAutoScale(bool IncludeBounds, bool ExcludeBounds)
	{
		bool Suspended = false;

		if (this->_AutoScaleMode != AutoScaleMode::None && this->_AutoScaleMode != AutoScaleMode::Inherit)
		{
			// TODO: SuspendAllLayout(this)
			Suspended = true;

			Drawing::SizeF Included{};
			Drawing::SizeF Excluded{};

			if (IncludeBounds)
				Included = this->AutoScaleFactor();
			if (ExcludeBounds)
				Excluded = this->AutoScaleFactor();

			this->Scale(Included, Excluded, this);
			this->_AutoScaleDimensions = this->CurrentAutoScaleDimensions();
		}

		if (IncludeBounds)
		{
			this->_ScalingNeededOnLayout = false;
			this->EnableRequiredScaling(this, false);
		}

		if (Suspended)
		{
			// TODO: ResumeAllLayout(this, false);
		}
	}

	Drawing::SizeF ContainerControl::GetFontAutoScaleDimensions()
	{
		Drawing::SizeF Result{};

		auto hDC = CreateCompatibleDC(nullptr);
		auto CurrentFont = this->GetFont();
		auto OldFont = SelectObject(hDC, CurrentFont->GetFontHandle());

		TEXTMETRICA Tm{};
		GetTextMetricsA(hDC, &Tm);

		Result.Height = (float)Tm.tmHeight;

		if ((Tm.tmPitchAndFamily & TMPF_FIXED_PITCH) != 0)
		{
			constexpr const char* FontMeasureString = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
			const uint32_t FontMeasureSize = (uint32_t)strlen(FontMeasureString);

			SIZE Sz{};
			GetTextExtentPoint32A(hDC, FontMeasureString, FontMeasureSize, &Sz);
			Result.Width = (float)(int)std::roundf(((float)Sz.cx) / ((float)FontMeasureSize));
		}
		else
		{
			Result.Width = (float)Tm.tmAveCharWidth;
		}

		SelectObject(hDC, OldFont);
		DeleteDC(hDC);

		return Result;
	}

	bool ContainerControl::ActivateControlInternal(Control* Ctrl, bool Originator)
	{
		bool Result = true;
		bool UpdateContainerActiveControl = false;

		ContainerControl* Cc = nullptr;
		Control* Parent = this->_Parent;

		if (Parent != nullptr)
		{
			Cc = (ContainerControl*)Parent->GetContainerControl();

			if (Cc != nullptr)
			{
				UpdateContainerActiveControl = (Cc->ActiveControl() != this);
			}
		}

		if (Ctrl != _ActiveControl || UpdateContainerActiveControl)
		{
			if (UpdateContainerActiveControl)
			{
				if (!Cc->ActivateControlInternal(this, false))
					return false;
			}

			Result = AssignActiveControlInternal((Ctrl == this) ? nullptr : Ctrl);
		}

		if (Originator)
		{
			// TODO: ScrollActiveControlIntoView();
		}

		return Result;
	}

	bool ContainerControl::AssignActiveControlInternal(Control* Ctrl)
	{
		if (_ActiveControl != Ctrl)
		{
			_ActiveControl = Ctrl;
			// TODO: UpdateFocusedControl();

			if (_ActiveControl == Ctrl)
			{
				auto FormCtrl = (Form*)FindForm();
				if (FormCtrl != nullptr)
					FormCtrl->UpdateDefaultButton();
			}
		}
		else
		{
			_FocusedControl = _ActiveControl;
		}

		return (_ActiveControl == Ctrl);
	}

	void ContainerControl::Select(bool Directed, bool Forward)
	{
		bool CorrectParentActiveControl = true;
		if (this->_Parent != nullptr)
		{
			auto C = (ContainerControl*)this->_Parent->GetContainerControl();
			if (C != nullptr)
			{
				C->SetActiveControl(this);
				CorrectParentActiveControl = (C->ActiveControl() == this);
			}
		}

		if (Directed && CorrectParentActiveControl)
			SelectNextControl(nullptr, Forward, true, true, false);
	}

	void ContainerControl::Scale(Drawing::SizeF IncludedFactor, Drawing::SizeF ExcludedFactor, Control* Ctrl)
	{
		if (this->_AutoScaleMode == AutoScaleMode::Inherit)
		{
			Control::Scale(IncludedFactor, ExcludedFactor, Ctrl);
		}
		else
		{
			Drawing::SizeF OurExcludedFactor = ExcludedFactor;
			Drawing::SizeF ChildIncludedFactor = IncludedFactor;

			if (!OurExcludedFactor.Empty())
				OurExcludedFactor = this->AutoScaleFactor();

			// If we're not supposed to be scaling, don't scale the internal ones either.
			if (this->AutoScaleMode() == AutoScaleMode::None)
				ChildIncludedFactor = this->AutoScaleFactor();

			Drawing::SizeF OurExternalContainerFactor = OurExcludedFactor;

			if (!ExcludedFactor.Empty() && this->_Parent != nullptr)
			{
				OurExternalContainerFactor = Drawing::SizeF{};

				if ((Ctrl != this))
				{
					OurExternalContainerFactor = ExcludedFactor;
				}
			}

			ScaleControl(IncludedFactor, OurExternalContainerFactor, Ctrl);
			ScaleChildControls(ChildIncludedFactor, OurExcludedFactor, Ctrl);
		}
	}

	void ContainerControl::OnLayoutResuming(bool PerformLayout)
	{
		this->PerformNeededAutoScaleOnLayout();
		Control::OnLayoutResuming(PerformLayout);
	}

	void ContainerControl::OnChildLayoutResuming(Control* Child, bool PerformLayout)
	{
		Control::OnChildLayoutResuming(Child, PerformLayout);

		// Do not scale children if AutoScaleMode is set to Dpi
		if (/*DpiHelper.EnableSinglePassScalingOfDpiForms &&*/ (this->_AutoScaleMode == AutoScaleMode::Dpi))
		{
			return;
		}

		// Perform scaling of the child control
		if (!PerformLayout && this->_AutoScaleMode != AutoScaleMode::None && this->_AutoScaleMode != AutoScaleMode::Inherit && this->_ScalingNeededOnLayout)
		{
			Child->Scale(this->AutoScaleFactor(), Drawing::SizeF{}, this);
		}
	}

	void ContainerControl::FocusActiveControlInternal()
	{
		if (_ActiveControl != nullptr && _ActiveControl->Visible())
		{
			auto FocusHandle = GetFocus();
			if (FocusHandle == NULL || FocusHandle != _ActiveControl->GetHandle())
				SetFocus(_ActiveControl->GetHandle());
		}
		else
		{
			ContainerControl* Cc = this;
			while (Cc != nullptr && !Cc->Visible())
			{
				auto Parent = Cc->Parent();
				if (Parent != nullptr)
					Cc = (ContainerControl*)Parent->GetContainerControl();
				else
					break;
			}

			if (Cc != nullptr && Cc->Visible())
				SetFocus(Cc->GetHandle());
		}
	}

	void ContainerControl::SetActiveControlInternal(Control* Value)
	{
		if (_ActiveControl != Value || (Value != nullptr && !Value->Focused()))
		{
			bool Result = false;
			ContainerControl* Cc = nullptr;

			if (Value != nullptr && Value->Parent() != nullptr)
			{
				Cc = (ContainerControl*)Value->Parent()->GetContainerControl();
			}
			
			if (Cc != nullptr)
			{
				Result = ActivateControlInternal(Value, false);
			}
			else
			{
				Result = AssignActiveControlInternal(Value);
			}

			if (Cc != nullptr && Result)
			{
				ContainerControl* CcAncestor = this;
				while (CcAncestor->_Parent != nullptr && CcAncestor->_Parent->GetContainerControl() != nullptr)
					CcAncestor = (ContainerControl*)CcAncestor->_Parent->GetContainerControl();

				if (CcAncestor->ContainsFocus() /*&& Value is NOT USERCONTROL!!*/)
					Cc->FocusActiveControlInternal();
			}
		}
	}

	Control* ContainerControl::ParentFormInternal()
	{
		if (this->_Parent != nullptr)
			return this->_Parent->FindForm();

		if (this->_RTTI == ControlTypes::Form)
			return nullptr;

		return FindForm();
	}

	Drawing::SizeF ContainerControl::AutoScaleFactor()
	{
		auto Current = this->CurrentAutoScaleDimensions();
		auto Saved = this->AutoScaleDimensions();

		if (Saved.Empty())
		{
			return Drawing::SizeF(1.f, 1.f);
		}

		return Drawing::SizeF(Current.Width / Saved.Width, Current.Height / Saved.Height);
	}
}
