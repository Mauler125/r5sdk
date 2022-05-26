#include "stdafx.h"
#include "Form.h"
#include "Application.h"
#include "Button.h"

namespace Forms
{
	Form::Form()
		: ContainerControl(), _Owner(nullptr), _FormBorderStyle(FormBorderStyle::Sizable), _FormWindowState(FormWindowState::Normal), _FormStartPosition(FormStartPosition::WindowsDefaultLocation), _DialogResult(DialogResult::None), _CloseReason(CloseReason::None), _Opacity(255), _Icon(nullptr), _FormStateFlags(0x1f), _DefaultButton(nullptr)
	{
		// Default form styles
		SetStyle(ControlStyles::ContainerControl, true);
		SetState(ControlStates::StateVisible, false);
		SetState(ControlStates::StateTopLevel, true);

#if 0
		// This is used to debug the form state flags, we use the magic number 0x1f right now...
		_FormStateFlags.set(0, true);	// ControlBox
		_FormStateFlags.set(1, true);	// MaximizeBox
		_FormStateFlags.set(2, true);	// MinimizeBox
		_FormStateFlags.set(3, true);	// ShowInTaskbar
		_FormStateFlags.set(4, true);	// ShowIcon
		_FormStateFlags.set(5, false);	// TopMost
		_FormStateFlags.set(6, false);	// CalledClosing
		_FormStateFlags.set(7, false);	// CalledCreateControl
		_FormStateFlags.set(8, false);	// CalledMakeVisible
		_FormStateFlags.set(9, false);	// CalledOnLoad
		_FormStateFlags.set(10, false);	// IsClosing
		_FormStateFlags.set(11, false);	// ShowWindowOnCreate
		_FormStateFlags.set(12, false);	// UseCustomFrame
#endif

		// Setup the container
		this->_Controls = std::make_unique<ControlCollection>();

		// Default form background
		this->SetBackColor(Drawing::GetSystemColor(Drawing::SystemColors::Control));

		// We are a form control
		this->_RTTI = ControlTypes::Form;
	}

	FormBorderStyle Form::GetFormBorderStyle()
	{
		return this->_FormBorderStyle;
	}

	void Form::SetFormBorderStyle(FormBorderStyle Value)
	{
		this->_FormBorderStyle = Value;
		UpdateStyles();
	}

	bool Form::ControlBox()
	{
		return this->_FormStateFlags[0];
	}

	void Form::SetControlBox(bool Value)
	{
		this->_FormStateFlags[0] = Value;
		UpdateStyles();
	}

	DialogResult Form::GetDialogResult()
	{
		return this->_DialogResult;
	}

	void Form::SetDialogResult(DialogResult Value)
	{
		this->_DialogResult = Value;
	}

	bool Form::MaximizeBox()
	{
		return this->_FormStateFlags[1];
	}

	void Form::SetMaximizeBox(bool Value)
	{
		this->_FormStateFlags[1] = Value;
		UpdateStyles();
	}

	bool Form::MinimizeBox()
	{
		return this->_FormStateFlags[2];
	}

	void Form::SetMinimizeBox(bool Value)
	{
		this->_FormStateFlags[2] = Value;
		UpdateStyles();
	}

	bool Form::ShowInTaskbar()
	{
		return this->_FormStateFlags[3];
	}

	void Form::SetShowInTaskbar(bool Value)
	{
		this->_FormStateFlags[3] = Value;
		UpdateStyles();
	}

	bool Form::ShowIcon()
	{
		return this->_FormStateFlags[4];
	}

	void Form::SetShowIcon(bool Value)
	{
		this->_FormStateFlags[4] = Value;
		UpdateStyles();
	}

	std::unique_ptr<Drawing::Icon>& Form::Icon()
	{
		return this->_Icon;
	}

	void Form::SetIcon(std::unique_ptr<Drawing::Icon>&& Value)
	{
		this->_Icon = std::move(Value);

		if (GetState(ControlStates::StateCreated))
		{
			if (this->_Icon != nullptr)
			{
				SendMessageA(this->_Handle, WM_SETICON, ICON_SMALL, (LPARAM)this->_Icon->SmallHandle());
				SendMessageA(this->_Handle, WM_SETICON, ICON_BIG, (LPARAM)this->_Icon->LargeHandle());
			}
			else
			{
				SendMessageA(this->_Handle, WM_SETICON, ICON_SMALL, NULL);
				SendMessageA(this->_Handle, WM_SETICON, ICON_BIG, NULL);
			}
		}
	}

	FormStartPosition Form::StartPosition()
	{
		return this->_FormStartPosition;
	}

	void Form::SetStartPosition(FormStartPosition Value)
	{
		this->_FormStartPosition = Value;
	}

	bool Form::TopMost()
	{
		return this->_FormStateFlags[5];
	}

	void Form::SetTopMost(bool Value)
	{
		if (GetState(ControlStates::StateCreated))
		{
			auto Key = (Value) ? HWND_TOPMOST : HWND_NOTOPMOST;
			SetWindowPos(this->_Handle, Key, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		}

		this->_FormStateFlags[5] = Value;
	}

	FormWindowState Form::WindowState()
	{
		return this->_FormWindowState;
	}

	void Form::SetWindowState(FormWindowState Value)
	{
		switch (Value)
		{
		case FormWindowState::Normal:
			SetState(ControlStates::StateSizeLockedByOS, false);
			break;
		case FormWindowState::Maximized:
		case FormWindowState::Minimized:
			SetState(ControlStates::StateSizeLockedByOS, true);
			break;
		}

		if (GetState(ControlStates::StateCreated) && Visible())
		{
			switch (Value)
			{
			case FormWindowState::Normal:
				ShowWindow(this->_Handle, SW_NORMAL);
				break;
			case FormWindowState::Maximized:
				ShowWindow(this->_Handle, SW_MAXIMIZE);
				break;
			case FormWindowState::Minimized:
				ShowWindow(this->_Handle, SW_MINIMIZE);
				break;
			}
		}

		this->_FormWindowState = Value;
	}

	void Form::Activate()
	{
		if (Visible() && GetState(ControlStates::StateCreated))
			SetActive(true);
	}

	bool Form::CheckCloseDialog(bool ClosingOnly)
	{
		if (_DialogResult == DialogResult::None && Visible())
			return false;

		if (!CalledClosing())
		{
			auto EventArgs = std::make_unique<FormClosingEventArgs>(_CloseReason, false);

			OnClosing(EventArgs);
			OnFormClosing(EventArgs);

			if (EventArgs->Cancel)
				_DialogResult = DialogResult::None;
			else
				SetCalledClosing(true);
		}

		if (!ClosingOnly && _DialogResult != DialogResult::None)
		{
			auto EventArgs = std::make_unique<FormClosedEventArgs>(_CloseReason);

			OnClosed(EventArgs);
			OnFormClosed(EventArgs);

			SetCalledClosing(false);
		}

		if (_DialogResult != DialogResult::None)
			return true;

		return !Visible();
	}

	void Form::Close()
	{
		if (GetState(ControlStates::StateCreated))
		{
			_CloseReason = CloseReason::UserClosing;
			SendMessageA(this->_Handle, WM_CLOSE, NULL, NULL);
		}
	}

	DialogResult Form::ShowDialog()
	{
		return this->ShowDialog(nullptr);
	}

	DialogResult Form::ShowDialog(Form* Owner)
	{
		SetCalledOnLoad(false);
		SetCalledMakeVisible(false);

		this->_CloseReason = CloseReason::None;
		
		auto HwndCapture = GetCapture();
		if (HwndCapture != NULL)
		{
			SendMessageA(HwndCapture, WM_CANCELMODE, NULL, NULL);
			ReleaseCapture();
		}

		// Workaround so we always have an owner
		auto HwndActive = GetActiveWindow();
		auto HwndOwner = (Owner == nullptr) ? HwndActive : Owner->GetHandle();
		auto OldOwner = this->_Owner;

		// Set result and create the control
		this->_DialogResult = DialogResult::None;
		this->CreateControl(nullptr);

		// If we don't have an owner, set one
		if (Owner == nullptr)
			SetWindowLongPtr(this->_Handle, GWLP_HWNDPARENT, (LONG_PTR)HwndOwner);
		else
			SetOwner(Owner);

		SetState(ControlStates::StateModal, true);

		// Run dialog, then make sure we're invisible
		Application::RunDialog(this);

		if (!IsWindow(HwndActive))
			HwndActive = HwndOwner;
		if (IsWindow(HwndActive) && IsWindowVisible(HwndActive))
			SetActiveWindow(HwndActive);
		else if (IsWindow(HwndOwner) && IsWindowVisible(HwndOwner))
			SetActiveWindow(HwndOwner);

		SetVisibleCore(false);

		if (GetState(ControlStates::StateCreated))
		{
			DestroyHandle();
			SetState(ControlStates::StateModal, false);
		}

		SetOwner(OldOwner);

		// Return our result
		return GetDialogResult();
	}

	Form* Form::Owner()
	{
		return this->_Owner;
	}

	void Form::SetOwner(Form* Value)
	{
		HWND HwndOwner = (Value != nullptr) ? Value->GetHandle() : NULL;

		SetWindowLongPtr(this->_Handle, GWLP_HWNDPARENT, (LONG_PTR)HwndOwner);

		this->_Owner = Value;
	}

	bool Form::UseCustomFrame()
	{
		return this->_FormStateFlags[12];
	}

	void Form::SetUseCustomFrame(bool Value)
	{
		this->_FormStateFlags[12] = Value;
		this->Invalidate(true);
	}

	bool Form::Active()
	{
		auto ParentForm = (Form*)this->ParentFormInternal();
		if (ParentForm == nullptr)
			return this->_FormStateFlags[13];

		return (ParentForm->ActiveControl() == this && ParentForm->Active());
	}

	void Form::SetActive(bool Value)
	{
		if (this->_FormStateFlags[13] != Value)
		{
			this->_FormStateFlags[13] = Value;

			if (Value)
			{
				if (ActiveControl() == nullptr)
					SelectNextControl(nullptr, true, true, true, false);

				OnActivated();
			}
			else
			{
				OnDeactivate();
			}
		}
	}

	uint8_t Form::Opacity()
	{
		return this->_Opacity;
	}

	void Form::SetOpacity(uint8_t Value)
	{
		if (_Opacity != Value)
		{
			_Opacity = Value;
			
			if (GetState(ControlStates::StateCreated))
			{
				UpdateStyles();
				SetLayeredWindowAttributes(this->_Handle, 0, (BYTE)_Opacity, LWA_ALPHA);
			}
		}
	}

	void Form::UpdateDefaultButton()
	{
		if (this->ActiveControl() != nullptr && this->ActiveControl()->GetType() == ControlTypes::Button)
		{
			SetDefaultButton(this->ActiveControl());
		}
		else
		{
			SetDefaultButton(nullptr);
			// TODO: SetDefaultButton(AcceptButton);
		}
	}

	void Form::OnLoad()
	{
		// If we are a model window, we can set the position
		if (GetState(ControlStates::StateModal))
		{
			if (this->_FormStartPosition == FormStartPosition::CenterParent)
				CenterToParent();
			else if (this->_FormStartPosition == FormStartPosition::CenterScreen)
				CenterToScreen();
		}

		// If we have no icon set, set the exe one...
		// this logic matches WPF windows...
		if (this->_Icon == nullptr)
			this->SetIcon(Drawing::Icon::ApplicationIcon());

		Load.RaiseEvent(this);
	}

	void Form::OnHandleCreated()
	{
		// Change opacity if need be
		SetLayeredWindowAttributes(this->_Handle, NULL, (BYTE)_Opacity, LWA_ALPHA);

		// Ensure the icon is set
		if (this->_Icon != nullptr)
		{
			SendMessageA(this->_Handle, WM_SETICON, ICON_SMALL, (LPARAM)this->_Icon->SmallHandle());
			SendMessageA(this->_Handle, WM_SETICON, ICON_BIG, (LPARAM)this->_Icon->LargeHandle());
		}

		// We must call the base event last
		ContainerControl::OnHandleCreated();
	}

	void Form::OnStyleChanged()
	{
		ContainerControl::OnStyleChanged();
		AdjustSystemMenu();
	}

	void Form::OnEnabledChanged()
	{
		ContainerControl::OnEnabledChanged();

		if (Enabled() && Active())
		{
			if (ActiveControl() == nullptr)
				SelectNextControl(this, true, true, true, true);
			else
				FocusActiveControlInternal();
		}
	}

	void Form::OnActivated()
	{
		Activated.RaiseEvent(this);
	}

	void Form::OnDeactivate()
	{
		Deactivate.RaiseEvent(this);
	}

	void Form::OnClosing(const std::unique_ptr<FormClosingEventArgs>& EventArgs)
	{
		Closing.RaiseEvent(EventArgs, this);
	}

	void Form::OnFormClosing(const std::unique_ptr<FormClosingEventArgs>& EventArgs)
	{
		FormClosing.RaiseEvent(EventArgs, this);
	}

	void Form::OnClosed(const std::unique_ptr<FormClosedEventArgs>& EventArgs)
	{
		Closed.RaiseEvent(EventArgs, this);
	}

	void Form::OnFormClosed(const std::unique_ptr<FormClosedEventArgs>& EventArgs)
	{
		FormClosed.RaiseEvent(EventArgs, this);
	}

	void Form::OnPaintFrame(const std::unique_ptr<PaintFrameEventArgs>& EventArgs)
	{
		PaintFrame.RaiseEvent(EventArgs, this);
	}

	void Form::AddControl(Control* Ctrl)
	{
		ContainerControl::AddControl(Ctrl);
	}

	void Form::Dispose()
	{
		// Forcefully set to cancel if we don't have a proper result...
		if (_DialogResult == DialogResult::None)
			_DialogResult = DialogResult::Cancel;

		// Call the base event last...
		ContainerControl::Dispose();
	}

	void Form::WndProc(Message& Msg)
	{
		switch (Msg.Msg)
		{
		case WM_CREATE:
			WmCreate(Msg);
			break;
		case WM_CLOSE:
			if (_CloseReason == CloseReason::None)
				_CloseReason = CloseReason::TaskManagerClosing;
			WmClose(Msg);
			break;
		case WM_QUERYENDSESSION:
		case WM_ENDSESSION:
			_CloseReason = CloseReason::WindowsShutDown;
			WmClose(Msg);
			break;
		case WM_SYSCOMMAND:
			WmSysCommand(Msg);
			break;
		case WM_GETMINMAXINFO:
			WmGetMinMaxInfo(Msg);
			break;
		case WM_ERASEBKGND:
			WmEraseBkgnd(Msg);
			break;
		case WM_WINDOWPOSCHANGED:
			WmWindowPosChanged(Msg);
			break;
		case WM_NCPAINT:
			if (UseCustomFrame())
				WmNcPaint(Msg);
			else
				ContainerControl::WndProc(Msg);
			break;
		case WM_NCACTIVATE:
			if (UseCustomFrame())
				WmNcActivate(Msg);
			else
				ContainerControl::WndProc(Msg);
			break;
		case WM_ACTIVATE:
			WmActivate(Msg);
			break;
		default:
			ContainerControl::WndProc(Msg);
			break;
		}
	}

	CreateParams Form::GetCreateParams()
	{
		auto Cp = ContainerControl::GetCreateParams();

		Cp.ClassName = "APPWINDOW";

		if (this->_Opacity < 255)
			Cp.ExStyle |= WS_EX_LAYERED;

		// Fill in properties
		FillInCreateParamsBorderStyles(Cp);
		FillInCreateParamsWindowState(Cp);
		FillInCreateParamsBorderIcons(Cp);

		if (ShowInTaskbar())
			Cp.ExStyle |= WS_EX_APPWINDOW;

		auto BorderStyle = GetFormBorderStyle();
		if (!ShowIcon() &&
			(BorderStyle == FormBorderStyle::Sizable ||
				BorderStyle == FormBorderStyle::Fixed3D ||
				BorderStyle == FormBorderStyle::FixedSingle))
		{
			Cp.ExStyle |= WS_EX_DLGMODALFRAME;
		}

		if (BorderStyle == FormBorderStyle::None)
			Cp.Style |= WS_POPUP;

		if (GetState(ControlStates::StateTopLevel))
		{
			FillInCreateParamsStartPosition(Cp);

			if ((Cp.Style & WS_VISIBLE) != 0)
			{
				SetShowWindowOnCreate(true);
				Cp.Style &= (~WS_VISIBLE);
			}
			else
			{
				SetShowWindowOnCreate(false);
			}
		}

		return Cp;
	}

	bool Form::CalledClosing()
	{
		return this->_FormStateFlags[6];
	}

	void Form::SetCalledClosing(bool Value)
	{
		this->_FormStateFlags[6] = Value;
	}

	bool Form::CalledCreateControl()
	{
		return this->_FormStateFlags[7];
	}

	void Form::SetCalledCreateControl(bool Value)
	{
		this->_FormStateFlags[7] = Value;
	}

	bool Form::CalledMakeVisible()
	{
		return this->_FormStateFlags[8];
	}

	void Form::SetCalledMakeVisible(bool Value)
	{
		this->_FormStateFlags[8] = Value;
	}

	bool Form::CalledOnLoad()
	{
		return this->_FormStateFlags[9];
	}

	void Form::SetCalledOnLoad(bool Value)
	{
		this->_FormStateFlags[9] = Value;
	}

	bool Form::IsClosing()
	{
		return this->_FormStateFlags[10];
	}

	void Form::SetIsClosing(bool Value)
	{
		this->_FormStateFlags[10] = Value;
	}

	bool Form::ShowWindowOnCreate()
	{
		return this->_FormStateFlags[11];
	}

	void Form::SetShowWindowOnCreate(bool Value)
	{
		this->_FormStateFlags[11] = Value;
	}

	void Form::SetVisibleCore(bool Value)
	{
		if (GetVisibleCore() == Value && _DialogResult == DialogResult::OK)
			return;

		if (GetVisibleCore() == Value && (!Value || CalledMakeVisible()))
		{
			ContainerControl::SetVisible(Value);
			return;
		}

		if (Value)
		{
			SetCalledMakeVisible(true);

			if (!CalledCreateControl())
			{
				this->CreateControl(_Parent);
				this->SetCalledCreateControl(true);
			}

			if (!CalledOnLoad())
			{
				SetCalledOnLoad(true);
				OnLoad();
				if (_DialogResult != DialogResult::None)
					Value = false;
			}
		}

		// If not mdi child:
		ContainerControl::SetVisibleCore(Value);

		if (Value && (WindowState() == FormWindowState::Maximized || TopMost()))
		{
			if (ActiveControl() == nullptr)
				SelectNextControl(nullptr, true, true, true, false);
			FocusActiveControlInternal();
		}
	}

	void Form::WmClose(Message& Msg)
	{
		auto EventArgs = std::make_unique<FormClosingEventArgs>(_CloseReason, false);

		if (Msg.Msg != WM_ENDSESSION)
		{
			if (GetState(ControlStates::StateModal))
			{
				if (_DialogResult == DialogResult::None)
					_DialogResult = DialogResult::Cancel;

				SetCalledClosing(false);
				EventArgs->Cancel = !CheckCloseDialog(true);
			}
			else
			{
				OnClosing(EventArgs);
				OnFormClosing(EventArgs);
			}

			if (Msg.Msg == WM_QUERYENDSESSION)
				Msg.Result = (uintptr_t)(EventArgs->Cancel ? 0 : 1);

			if (GetState(ControlStates::StateModal))
				return;
		}
		else
		{
			EventArgs->Cancel = (Msg.WParam == (uintptr_t)0);
		}

		if (Msg.Msg != WM_QUERYENDSESSION)
		{
			if (!EventArgs->Cancel)
			{
				auto FcEventArgs = std::make_unique<FormClosedEventArgs>(CloseReason::FormOwnerClosing);

				SetIsClosing(true);

				FcEventArgs->Reason = _CloseReason;
				OnClosed(FcEventArgs);
				OnFormClosed(FcEventArgs);

				this->Dispose();	// Forcefully dispose of the control
			}
		}
	}

	void Form::WmSysCommand(Message& Msg)
	{
		bool CallDefault = true;

		auto Sc = (uint16_t)(LOWORD(Msg.WParam) & 0xFFF0);

		switch (Sc)
		{
		case SC_CLOSE:
			_CloseReason = CloseReason::UserClosing;
			break;
		}

		if (CallDefault)
			ContainerControl::WndProc(Msg);
	}

	void Form::WmEraseBkgnd(Message& Msg)
	{
		UpdateWindowState();
		ContainerControl::WndProc(Msg);
	}

	void Form::WmGetMinMaxInfo(Message& Msg)
	{
		auto MinTrack = this->MinimumSize();
		auto MaxTrack = this->MaximumSize();

		auto MinMaxInfo = (MINMAXINFO*)Msg.LParam;

		if (!MinTrack.Empty())
		{
			MinMaxInfo->ptMinTrackSize.x = MinTrack.Width;
			MinMaxInfo->ptMinTrackSize.y = MinTrack.Height;

			if (MaxTrack.Empty())
			{
				// Make sure we aren't bigger than our total screen size
				Drawing::Size VirtualScreenSize(0, 0);
				Drawing::Rectangle(GetSystemMetrics(SM_XVIRTUALSCREEN), GetSystemMetrics(SM_YVIRTUALSCREEN), GetSystemMetrics(SM_CXVIRTUALSCREEN), GetSystemMetrics(SM_CYVIRTUALSCREEN)).GetSize(&VirtualScreenSize);

				if (MinTrack.Height > VirtualScreenSize.Height)
					MinMaxInfo->ptMaxTrackSize.y = INT_MAX;
				if (MinTrack.Width > VirtualScreenSize.Width)
					MinMaxInfo->ptMaxTrackSize.x = INT_MAX;
			}
		}

		if (!MaxTrack.Empty())
		{
			MinMaxInfo->ptMaxTrackSize.x = MaxTrack.Width;
			MinMaxInfo->ptMaxTrackSize.y = MaxTrack.Height;
		}

		Msg.Result = (uintptr_t)0;
	}

	void Form::WmWindowPosChanged(Message& Msg)
	{
		UpdateWindowState();
		ContainerControl::WndProc(Msg);
	}

	void Form::WmNcPaint(Message& Msg)
	{
		auto InRgn = (HRGN)Msg.WParam;
		HDC DC = nullptr;

		if (InRgn != (HRGN)1)
			DC = GetDCEx((HWND)Msg.HWnd, InRgn, DCX_WINDOW | DCX_USESTYLE | DCX_LOCKWINDOWUPDATE | DCX_INTERSECTRGN | DCX_NODELETERGN);
		else
			DC = GetDCEx((HWND)Msg.HWnd, InRgn, DCX_WINDOW | DCX_USESTYLE | DCX_LOCKWINDOWUPDATE);

		OnPaintFrame(std::make_unique<PaintFrameEventArgs>(DC, Drawing::Rectangle(0, 0, this->_Width, this->_Height), this->Active()));

		ReleaseDC((HWND)Msg.HWnd, DC);

		Msg.Result = (uintptr_t)0;
	}

	void Form::WmNcActivate(Message& Msg)
	{
		HRGN InRgn = nullptr;
		HDC DC = nullptr;
		auto Flags = DCX_WINDOW | DCX_USESTYLE | DCX_LOCKWINDOWUPDATE;

		InRgn = ContainerControl::CreateCopyOfRgn((HRGN)Msg.LParam);

		if (InRgn)
			Flags |= DCX_INTERSECTRGN;

		if ((DC = GetDCEx((HWND)Msg.HWnd, InRgn, Flags)) == NULL)
		{
			DeleteObject(InRgn);
			return;
		}

		RECT Rc;
		RECT RcWnd;
		GetWindowRect((HWND)Msg.HWnd, &RcWnd);
		GetClientRect((HWND)Msg.HWnd, &Rc);

		MapWindowPoints((HWND)Msg.HWnd, NULL, (LPPOINT)&Rc, 1);

		Rc.left -= RcWnd.left;
		Rc.top -= RcWnd.top;
		Rc.right += Rc.left;
		Rc.bottom += Rc.top;

		ExcludeClipRect(DC, Rc.left, Rc.top, Rc.right, Rc.bottom);

		OnPaintFrame(std::make_unique<PaintFrameEventArgs>(DC, Drawing::Rectangle(0, 0, this->_Width, this->_Height), (bool)Msg.WParam));

		if (InRgn)
			DeleteObject(InRgn);
		ReleaseDC((HWND)Msg.HWnd, DC);

		Msg.Result = (uintptr_t)1;	// Resume normal processing
	}

	void Form::WmActivate(Message& Msg)
	{
		this->SetActive((bool)((int16_t)LOWORD(Msg.WParam) != WA_INACTIVE));
		// TODO: Application.FormActivated(this.Modal, Value);
	}

	void Form::WmCreate(Message& Msg)
	{
		ContainerControl::WndProc(Msg);

		STARTUPINFOA Si{};
		GetStartupInfoA(&Si);

		if (GetState(ControlStates::StateTopLevel) && (Si.dwFlags & STARTF_USESHOWWINDOW) != 0)
		{
			switch (Si.wShowWindow)
			{
			case SW_MAXIMIZE:
				this->SetWindowState(FormWindowState::Maximized);
				break;
			case SW_MINIMIZE:
				this->SetWindowState(FormWindowState::Minimized);
				break;
			}
		}
	}

	void Form::FillInCreateParamsBorderStyles(CreateParams& Cp)
	{
		switch (_FormBorderStyle)
		{
		case FormBorderStyle::FixedSingle:
			Cp.Style |= WS_BORDER;
			break;
		case FormBorderStyle::Sizable:
			Cp.Style |= WS_BORDER | WS_THICKFRAME;
			break;
		case FormBorderStyle::Fixed3D:
			Cp.Style |= WS_BORDER;
			Cp.ExStyle |= WS_EX_CLIENTEDGE;
			break;
		case FormBorderStyle::FixedDialog:
			Cp.Style |= WS_BORDER;
			Cp.ExStyle |= WS_EX_DLGMODALFRAME;
			break;
		case FormBorderStyle::FixedToolWindow:
			Cp.Style |= WS_BORDER;
			Cp.ExStyle |= WS_EX_TOOLWINDOW;
			break;
		case FormBorderStyle::SizeableToolWindow:
			Cp.Style |= WS_BORDER | WS_THICKFRAME;
			Cp.ExStyle |= WS_EX_TOOLWINDOW;
			break;
		}
	}

	void Form::FillInCreateParamsWindowState(CreateParams& Cp)
	{
		switch (_FormWindowState)
		{
		case FormWindowState::Maximized:
			Cp.Style |= WS_MAXIMIZE;
			break;
		case FormWindowState::Minimized:
			Cp.Style |= WS_MINIMIZE;
			break;
		}
	}

	void Form::FillInCreateParamsBorderIcons(CreateParams& Cp)
	{
		if (_FormBorderStyle == FormBorderStyle::None)
			return;

		if (Text().Length() != 0)
			Cp.Style |= WS_CAPTION;

		if (ControlBox())
			Cp.Style |= WS_SYSMENU | WS_CAPTION;
		else
			Cp.Style &= (~WS_SYSMENU);

		if (MaximizeBox())
			Cp.Style |= WS_MAXIMIZEBOX;
		else
			Cp.Style &= (~WS_MAXIMIZEBOX);

		if (MinimizeBox())
			Cp.Style |= WS_MINIMIZEBOX;
		else
			Cp.Style &= (~WS_MINIMIZEBOX);

		Cp.ExStyle &= (~WS_EX_CONTEXTHELP);
	}

	void Form::FillInCreateParamsStartPosition(CreateParams& Cp)
	{
		switch (_FormStartPosition)
		{
		case FormStartPosition::WindowsDefaultBounds:
			Cp.Width = CW_USEDEFAULT;
			Cp.Height = CW_USEDEFAULT;
			Cp.X = CW_USEDEFAULT;
			Cp.Y = CW_USEDEFAULT;
			break;
		case FormStartPosition::WindowsDefaultLocation:
		case FormStartPosition::CenterParent:
			Cp.X = CW_USEDEFAULT;
			Cp.Y = CW_USEDEFAULT;
			break;
		case FormStartPosition::CenterScreen:
			if (_FormWindowState != FormWindowState::Maximized)
			{
				// Calculate X / Y based on our width/height
				HMONITOR MonHandle = nullptr;

				if (_Parent != nullptr)
					MonHandle = MonitorFromWindow(_Parent->GetHandle(), MONITOR_DEFAULTTONEAREST);
				else
				{
					auto CursorPos = ContainerControl::GetMousePosition();
					MonHandle = MonitorFromPoint({ CursorPos.X, CursorPos.Y }, MONITOR_DEFAULTTONEAREST);
				}

				if (MonHandle != nullptr)
				{
					MONITORINFO Info;
					Info.cbSize = sizeof(Info);
					GetMonitorInfo(MonHandle, &Info);

					Cp.X = max((uint32_t)Info.rcWork.left, (Info.rcWork.left + Info.rcWork.right) / 2 - Cp.Width / 2);
					Cp.Y = max((uint32_t)Info.rcWork.top, (Info.rcWork.top + Info.rcWork.bottom) / 2 - Cp.Height / 2);
				}
			}
			break;
		}
	}

	void Form::UpdateWindowState()
	{
		if (!GetState(ControlStates::StateCreated))
			return;

		auto OldState = WindowState();

		WINDOWPLACEMENT Wp{};
		Wp.length = sizeof(WINDOWPLACEMENT);
		GetWindowPlacement(this->_Handle, &Wp);

		switch (Wp.showCmd)
		{
		case SW_NORMAL:
		case SW_RESTORE:
		case SW_SHOW:
		case SW_SHOWNA:
		case SW_SHOWNOACTIVATE:
			_FormWindowState = FormWindowState::Normal;
			break;
		case SW_SHOWMAXIMIZED:
			_FormWindowState = FormWindowState::Maximized;
			break;
		case SW_SHOWMINIMIZED:
		case SW_MINIMIZE:
		case SW_SHOWMINNOACTIVE:
			_FormWindowState = FormWindowState::Minimized;
			break;
		}

		switch (WindowState())
		{
		case FormWindowState::Normal:
			SetState(ControlStates::StateSizeLockedByOS, false);
			break;
		case FormWindowState::Maximized:
		case FormWindowState::Minimized:
			SetState(ControlStates::StateSizeLockedByOS, true);
			break;
		}

		if (OldState != WindowState())
			AdjustSystemMenu();
	}

	void Form::AdjustSystemMenu()
	{
		if (GetState(ControlStates::StateCreated))
		{
			auto hMenu = GetSystemMenu(this->_Handle, false);
			AdjustSystemMenu(hMenu);
		}
	}

	void Form::AdjustSystemMenu(HMENU Menu)
	{
		UpdateWindowState();

		auto WinState = WindowState();
		auto BorderStyle = GetFormBorderStyle();
		bool SizableBorder = (BorderStyle == FormBorderStyle::SizeableToolWindow || BorderStyle == FormBorderStyle::Sizable);

		bool ShowMin = MinimizeBox() && WinState != FormWindowState::Minimized;
		bool ShowMax = MaximizeBox() && WinState != FormWindowState::Maximized;
		bool ShowClose = ControlBox();
		bool ShowRestore = WinState != FormWindowState::Normal;
		bool ShowSize = SizableBorder && WinState != FormWindowState::Minimized && WinState != FormWindowState::Maximized;

		if (!ShowMin)
			EnableMenuItem(Menu, SC_MINIMIZE, MF_BYCOMMAND | MF_GRAYED);
		else
			EnableMenuItem(Menu, SC_MINIMIZE, MF_BYCOMMAND | MF_ENABLED);
		if (!ShowMax)
			EnableMenuItem(Menu, SC_MAXIMIZE, MF_BYCOMMAND | MF_GRAYED);
		else
			EnableMenuItem(Menu, SC_MAXIMIZE, MF_BYCOMMAND | MF_ENABLED);
		if (!ShowClose)
			EnableMenuItem(Menu, SC_CLOSE, MF_BYCOMMAND | MF_GRAYED);
		else
			EnableMenuItem(Menu, SC_CLOSE, MF_BYCOMMAND | MF_ENABLED);
		if (!ShowRestore)
			EnableMenuItem(Menu, SC_RESTORE, MF_BYCOMMAND | MF_GRAYED);
		else
			EnableMenuItem(Menu, SC_RESTORE, MF_BYCOMMAND | MF_ENABLED);
		if (!ShowSize)
			EnableMenuItem(Menu, SC_SIZE, MF_BYCOMMAND | MF_GRAYED);
		else
			EnableMenuItem(Menu, SC_SIZE, MF_BYCOMMAND | MF_ENABLED);
	}

	void Form::SetDefaultButton(Control* Btn)
	{
		if (_DefaultButton != Btn)
		{
			if (_DefaultButton != nullptr)
				((Button*)_DefaultButton)->NotifyDefault(false);

			_DefaultButton = Btn;

			if (Btn != nullptr)
				((Button*)Btn)->NotifyDefault(true);
		}
	}

	void Form::CenterToParent()
	{
		auto HwndOwner = (HWND)GetWindowLongPtr(this->_Handle, GWLP_HWNDPARENT);

		if (HwndOwner != NULL)
		{
			auto Screen = MonitorFromWindow(HwndOwner, MONITOR_DEFAULTTONEAREST);

			MONITORINFO Mi;
			Mi.cbSize = sizeof(Mi);
			GetMonitorInfo(Screen, &Mi);

			RECT OwnerRect{};
			GetWindowRect(HwndOwner, &OwnerRect);

			auto SizeCache = this->Size();
			Drawing::Point Point(0, 0);

			Point.X = (OwnerRect.left + OwnerRect.right - SizeCache.Width) / 2;
			if (Point.X < Mi.rcWork.left)
				Point.X = Mi.rcWork.left;
			else if (Point.X + SizeCache.Width > Mi.rcWork.right)
				Point.X = Mi.rcWork.right - SizeCache.Width;

			Point.Y = (OwnerRect.top + OwnerRect.bottom - SizeCache.Height) / 2;
			if (Point.Y < Mi.rcWork.top)
				Point.Y = Mi.rcWork.top;
			else if (Point.Y + SizeCache.Height > Mi.rcWork.bottom)
				Point.Y = Mi.rcWork.bottom - SizeCache.Height;

			this->SetLocation(Point);
		}
		else
		{
			CenterToScreen();
		}
	}

	void Form::CenterToScreen()
	{
		Drawing::Point Point(0, 0);
		HMONITOR Desktop = nullptr;

		if (_Owner != nullptr)
		{
			Desktop = MonitorFromWindow(_Owner->GetHandle(), MONITOR_DEFAULTTONEAREST);
		}
		else
		{
			auto HwndOwner = (HWND)GetWindowLongPtr(this->_Handle, GWLP_HWNDPARENT);

			if (HwndOwner != NULL)
				Desktop = MonitorFromWindow(HwndOwner, MONITOR_DEFAULTTONEAREST);
			else
			{
				auto Cursor = ContainerControl::GetMousePosition();
				Desktop = MonitorFromPoint({ Cursor.X, Cursor.Y }, MONITOR_DEFAULTTONEAREST);
			}
		}

		MONITORINFO Info;
		Info.cbSize = sizeof(Info);
		GetMonitorInfo(Desktop, &Info);

		Point.X = max((uint32_t)Info.rcWork.left, (Info.rcWork.left + Info.rcWork.right) / 2 - this->_Width / 2);
		Point.Y = max((uint32_t)Info.rcWork.top, (Info.rcWork.top + Info.rcWork.bottom) / 2 - this->_Height / 2);

		this->SetLocation(Point);
	}
}
