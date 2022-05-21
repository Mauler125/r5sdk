#include "stdafx.h"
#include "ToolTip.h"

namespace Forms
{
	ToolTip::ToolTip()
		: Control(), _Flags(0x33), _ToolTipIcon(ToolTipIcon::None), _DelayTimes{}
	{
#if 0
		// This is used to debug the tooltip state flags, we use the magic number 0x33 right now...
		_Flags.set(0, true);	// Active
		_Flags.set(1, true);	// Auto
		_Flags.set(2, false);	// ShowAlways
		_Flags.set(3, false);	// StripAmpersands
		_Flags.set(4, true);	// UseAnimation
		_Flags.set(5, true);	// UseFading
		_Flags.set(6, false);	// OwnerDraw
		_Flags.set(7, false);	// IsBalloon
		_Flags.set(8, false);	// TrackPosition
		_Flags.set(9, false);	// Cancelled
#endif

		// Set the automatic time default
		_DelayTimes[TTDT_AUTOMATIC] = DEFAULT_DELAY;
		// Adjust the base times from auto
		AdjustBaseFromAuto();

		// Default tooltip background and foreground
		this->SetBackColor(Drawing::GetSystemColor(Drawing::SystemColors::Info));
		this->SetForeColor(Drawing::GetSystemColor(Drawing::SystemColors::InfoText));

		// We are a tooltip control
		this->_RTTI = ControlTypes::ToolTip;
	}

	void ToolTip::CreateControl(Control* Parent)
	{
		INITCOMMONCONTROLSEX iCC{};
		iCC.dwSize = sizeof(iCC);
		iCC.dwICC = ICC_TAB_CLASSES;

		InitCommonControlsEx(&iCC);

		Control::CreateControl(Parent);
	}

	bool ToolTip::Active()
	{
		return this->_Flags[0];
	}

	void ToolTip::SetActive(bool Value)
	{
		if (Active() != Value)
		{
			this->_Flags[0] = Value;

			if (GetState(ControlStates::StateCreated))
				SendMessageA(this->_Handle, TTM_ACTIVATE, (Value) ? 1 : 0, NULL);
		}
	}

	uint32_t ToolTip::AutomaticDelay()
	{
		return _DelayTimes[TTDT_AUTOMATIC];
	}

	void ToolTip::SetAutomaticDelay(uint32_t Value)
	{
		SetDelayTime(TTDT_AUTOMATIC, Value);
	}

	uint32_t ToolTip::AutoPopDelay()
	{
		return _DelayTimes[TTDT_AUTOPOP];
	}

	void ToolTip::SetAutoPopDelay(uint32_t Value)
	{
		SetDelayTime(TTDT_AUTOPOP, Value);
	}

	bool ToolTip::IsBalloon()
	{
		return this->_Flags[7];
	}

	void ToolTip::SetIsBalloon(bool Value)
	{
		if (this->_Flags[7] != Value)
		{
			this->_Flags[7] = Value;
			UpdateStyles();
		}
	}

	uint32_t ToolTip::InitialDelay()
	{
		return _DelayTimes[TTDT_INITIAL];
	}

	void ToolTip::SetInitialDelay(uint32_t Value)
	{
		SetDelayTime(TTDT_INITIAL, Value);
	}

	bool ToolTip::OwnerDraw()
	{
		return this->_Flags[6];
	}

	void ToolTip::SetOwnerDraw(bool Value)
	{
		this->_Flags[6] = Value;
	}

	uint32_t ToolTip::ReshowDelay()
	{
		return _DelayTimes[TTDT_RESHOW];
	}

	void ToolTip::SetReshowDelay(uint32_t Value)
	{
		SetDelayTime(TTDT_RESHOW, Value);
	}

	bool ToolTip::ShowAlways()
	{
		return this->_Flags[2];
	}

	void ToolTip::SetShowAlways(bool Value)
	{
		if (this->_Flags[2] != Value)
		{
			this->_Flags[2] = Value;
			UpdateStyles();
		}
	}

	bool ToolTip::StripAmpersands()
	{
		return this->_Flags[3];
	}

	void ToolTip::SetStripAmpersands(bool Value)
	{
		if (this->_Flags[3] != Value)
		{
			this->_Flags[3] = Value;
			UpdateStyles();
		}
	}

	ToolTipIcon ToolTip::GetToolTipIcon()
	{
		return this->_ToolTipIcon;
	}

	void ToolTip::SetToolTipIcon(ToolTipIcon Value)
	{
		if (_ToolTipIcon != Value)
		{
			_ToolTipIcon = Value;

			if (GetState(ControlStates::StateCreated))
			{
				string Title = !string::IsNullOrEmpty(_ToolTipTitle) ? _ToolTipTitle : string(" ");

				SendMessageA(this->_Handle, TTM_SETTITLEA, (WPARAM)Value, (LPARAM)(char*)Title);
				SendMessageA(this->_Handle, TTM_UPDATE, NULL, NULL);
			}
		}
	}

	string ToolTip::ToolTipTitle()
	{
		return this->_ToolTipTitle;
	}

	void ToolTip::SetToolTipTitle(const string& Value)
	{
		if (_ToolTipTitle != Value)
		{
			_ToolTipTitle = Value;

			if (GetState(ControlStates::StateCreated))
			{
				SendMessageA(this->_Handle, TTM_SETTITLEA, (WPARAM)_ToolTipIcon, (LPARAM)(char*)_ToolTipTitle);
				SendMessageA(this->_Handle, TTM_UPDATE, NULL, NULL);
			}
		}
	}

	bool ToolTip::UseAnimation()
	{
		return this->_Flags[4];
	}

	void ToolTip::SetUseAnimation(bool Value)
	{
		if (this->_Flags[4] != Value)
		{
			this->_Flags[4] = Value;
			UpdateStyles();
		}
	}

	bool ToolTip::UseFading()
	{
		return this->_Flags[5];
	}

	void ToolTip::SetUseFading(bool Value)
	{
		if (this->_Flags[5] != Value)
		{
			this->_Flags[5] = Value;
			UpdateStyles();
		}
	}

	void ToolTip::SetToolTip(Control* Ctrl, const string& Caption)
	{
		if (Ctrl->GetState(ControlStates::StateCreated))
		{
			// Check if we already setup the control...
			if (this->_ControlCache.ContainsKey((uintptr_t)Ctrl))
			{
				TOOLINFOA Ti{};
				Ti.cbSize = sizeof(Ti);
				Ti.hwnd = Ctrl->GetHandle();
				Ti.uFlags = TTF_IDISHWND | TTF_TRANSPARENT | TTF_SUBCLASS;
				Ti.uId = (UINT_PTR)Ctrl->GetHandle();
				Ti.lpszText = (char*)Caption;

				SendMessageA(this->_Handle, TTM_UPDATETIPTEXTA, NULL, (LPARAM)&Ti);

				auto Ptr = (uintptr_t)Ctrl;
				this->_ControlCache[Ptr] = Caption;

				return;
			}
			
			// Add the item to the control cache
			this->_ControlCache.Add((uintptr_t)Ctrl, Caption);
			// Register the tooltip handler
			this->RegisterTooltip(Ctrl);

			// Hook if control gets deleted
			Ctrl->HandleDestroyed += &ToolTip::ControlHandleDestroyed;
		}
		else
		{
			// Add the item and wait for creation
			this->_ControlCache.Add((uintptr_t)Ctrl, Caption);
			Ctrl->HandleCreated += &ToolTip::ControlHandleCreated;
		}
	}

	void ToolTip::OnHandleCreated()
	{
		Control::OnHandleCreated();

		if (OwnerDraw())
		{
			auto Style = (int)GetWindowLong(this->_Handle, GWL_STYLE);
			Style &= WS_BORDER;
			SetWindowLong(this->_Handle, GWL_STYLE, Style);
		}

		SendMessageA(this->_Handle, TTM_SETMAXTIPWIDTH, NULL, GetSystemMetrics(SM_CXMAXTRACK));

		if (this->_Flags[1])
		{
			SetDelayTime(TTDT_AUTOMATIC, _DelayTimes[TTDT_AUTOMATIC]);
			_DelayTimes[TTDT_AUTOPOP] = GetDelayTime(TTDT_AUTOPOP);
			_DelayTimes[TTDT_INITIAL] = GetDelayTime(TTDT_INITIAL);
			_DelayTimes[TTDT_RESHOW] = GetDelayTime(TTDT_RESHOW);
		}
		else
		{
			for (int32_t i = 0; i < 4; i++)
			{
				if (_DelayTimes[i] > 1)
					SetDelayTime(i, _DelayTimes[i]);
			}
		}

		SendMessageA(this->_Handle, TTM_ACTIVATE, (Active() == true) ? 1 : 0, NULL);

		SendMessageA(this->_Handle, TTM_SETTIPBKCOLOR, Drawing::ColorToWin32(this->BackColor()), NULL);
		SendMessageA(this->_Handle, TTM_SETTIPTEXTCOLOR, Drawing::ColorToWin32(this->ForeColor()), NULL);

		if ((int)_ToolTipIcon > 0 || !string::IsNullOrEmpty(_ToolTipTitle))
		{
			string Title = !string::IsNullOrEmpty(_ToolTipTitle) ? _ToolTipTitle : string(" ");
			SendMessageA(this->_Handle, TTM_SETTITLEA, (WPARAM)_ToolTipIcon, (LPARAM)(char*)Title);
		}
	}

	void ToolTip::OnBackColorChanged()
	{
		if (GetState(ControlStates::StateCreated))
			SendMessageA(this->_Handle, TTM_SETTIPBKCOLOR, Drawing::ColorToWin32(this->BackColor()), NULL);

		Control::OnBackColorChanged();
	}

	void ToolTip::OnForeColorChanged()
	{
		if (GetState(ControlStates::StateCreated))
			SendMessageA(this->_Handle, TTM_SETTIPTEXTCOLOR, Drawing::ColorToWin32(this->ForeColor()), NULL);

		Control::OnForeColorChanged();
	}

	void ToolTip::OnPopup(const std::unique_ptr<PopupEventArgs>& EventArgs)
	{
		Popup.RaiseEvent(EventArgs, this);
	}

	void ToolTip::OnDraw(const std::unique_ptr<DrawToolTipEventArgs>& EventArgs)
	{
		Draw.RaiseEvent(EventArgs, this);
	}

	void ToolTip::RemoveAll()
	{
		for (auto& Kvp : this->_ControlCache)
		{
			auto Ctrl = (Control*)Kvp.Key();

			if (GetState(ControlStates::StateCreated) && Ctrl->GetState(ControlStates::StateCreated))
			{
				TOOLINFOA Ti{};
				Ti.cbSize = sizeof(Ti);
				Ti.hwnd = Ctrl->GetHandle();
				Ti.uFlags = TTF_IDISHWND | TTF_TRANSPARENT | TTF_SUBCLASS;
				Ti.uId = (UINT_PTR)Ctrl->GetHandle();

				SendMessageA(this->_Handle, TTM_DELTOOLA, NULL, (LPARAM)& Ti);
			}
		}

		this->_ControlCache.Clear();
	}

	void ToolTip::Remove(Control* Ctrl)
	{
		if (GetState(ControlStates::StateCreated) && Ctrl->GetState(ControlStates::StateCreated))
		{
			TOOLINFOA Ti{};
			Ti.cbSize = sizeof(Ti);
			Ti.hwnd = Ctrl->GetHandle();
			Ti.uFlags = TTF_IDISHWND | TTF_TRANSPARENT | TTF_SUBCLASS;
			Ti.uId = (UINT_PTR)Ctrl->GetHandle();

			SendMessageA(this->_Handle, TTM_DELTOOLA, NULL, (LPARAM)&Ti);
		}

		this->_ControlCache.Remove((uintptr_t)Ctrl);
	}

	void ToolTip::WndProc(Message& Msg)
	{
		switch (Msg.Msg)
		{
		case WM_REFLECT + WM_NOTIFY:
		{
			NMHDR* nMHDR = (NMHDR*)Msg.LParam;
			if (nMHDR->code == TTN_SHOW && !this->_Flags[8])
			{
				WmShow();
			}
			else if (nMHDR->code == TTN_POP)
			{
				WmPop();
				DefWndProc(Msg);
			}
		}
		break;
		case WM_WINDOWPOSCHANGING:
			DefWndProc(Msg);
			break;
		case WM_WINDOWPOSCHANGED:
			if (!WmWindowPosChanged())
				DefWndProc(Msg);
			break;
		case WM_MOVE:
			WmMove();
			break;
		case TTM_WINDOWFROMPOINT:
			WmWindowFromPoint(Msg);
			break;
		case WM_PRINTCLIENT:
		case WM_PAINT:
			if (OwnerDraw() && !IsBalloon() && !this->_Flags[8])
			{
				PAINTSTRUCT Ps{};
				auto Dc = BeginPaint(this->_Handle, &Ps);

				Drawing::Rectangle Bounds(Ps.rcPaint.left, Ps.rcPaint.top, Ps.rcPaint.right - Ps.rcPaint.left, Ps.rcPaint.bottom - Ps.rcPaint.top);

				if (Bounds.IsEmptyArea())
				{
					EndPaint(this->_Handle, &Ps);
					return;
				}

				TOOLINFOA Ti{};
				Ti.cbSize = sizeof(Ti);

				auto Result = (int32_t)SendMessageA(this->_Handle, TTM_GETCURRENTTOOLA, NULL, (LPARAM)&Ti);

				if (Result != 0)
				{
					auto Font = this->GetFont();
					auto Ctrl = Control::FromHandle(Ti.hwnd);
					auto CtrlPtr = (uintptr_t)Ctrl;

					auto& Value = this->_ControlCache[CtrlPtr];

					OnDraw(std::make_unique<DrawToolTipEventArgs>(Dc, Ctrl, Ctrl, Bounds, Value, BackColor(), ForeColor(), Font));
				}

				EndPaint(this->_Handle, &Ps);
			}
			else
			{
				DefWndProc(Msg);
			}
			break;
		default:
			DefWndProc(Msg);
			break;
		}
	}

	CreateParams ToolTip::GetCreateParams()
	{
		CreateParams Cp{};
		Cp.ClassName = "tooltips_class32";

		if (ShowAlways())
			Cp.Style = TTS_ALWAYSTIP;
		if (IsBalloon())
			Cp.Style |= TTS_BALLOON;
		if (!StripAmpersands())
			Cp.Style |= TTS_NOPREFIX;
		if (!UseAnimation())
			Cp.Style |= TTS_NOANIMATE;
		if (!UseFading())
			Cp.Style |= TTS_NOFADE;
		
		return Cp;
	}

	void ToolTip::RegisterTooltip(Control* Ctrl)
	{
		auto Ptr = (uintptr_t)Ctrl;
		auto Value = this->_ControlCache[Ptr];

		TOOLINFOA Ti{};
		Ti.cbSize = sizeof(Ti);
		Ti.hwnd = Ctrl->GetHandle();
		Ti.uFlags = TTF_IDISHWND | TTF_TRANSPARENT | TTF_SUBCLASS;
		Ti.uId = (UINT_PTR)Ctrl->GetHandle();
		Ti.lpszText = (char*)Value;

		SendMessageA(this->_Handle, TTM_ADDTOOLA, NULL, (LPARAM)&Ti);

		Ctrl->HandleDestroyed += &ToolTip::ControlHandleDestroyed;
	}

	void ToolTip::ControlHandleCreated(Control* Sender)
	{
		Sender->HandleCreated -= &ToolTip::ControlHandleCreated;

		// The tooltip should be a child of the form of the sender
		auto FormOwner = Sender->FindForm();
		if (FormOwner == nullptr)
			return;

		auto& ControlList = *FormOwner->Controls().get();

		for (auto& Control : ControlList)
		{
			if (Control.GetType() == ControlTypes::ToolTip)
			{
				auto Tt = (ToolTip*)&Control;

				if (Tt->_ControlCache.ContainsKey((uintptr_t)Sender))
				{
					Tt->RegisterTooltip(Sender);
					break;
				}
			}
		}
	}

	void ToolTip::ControlHandleDestroyed(Control* Sender)
	{
		Sender->HandleDestroyed -= &ToolTip::ControlHandleDestroyed;

		// The tooltip should be a child of the form of the sender
		auto FormOwner = Sender->FindForm();
		if (FormOwner == nullptr || FormOwner->GetState(ControlStates::StateDisposed))
			return;

		auto& ControlList = *FormOwner->Controls().get();

		for (auto& Control : ControlList)
		{
			if (Control.GetType() == ControlTypes::ToolTip)
			{
				auto Tt = (ToolTip*)&Control;

				if (Tt->GetState(ControlStates::StateCreated) && Tt->_ControlCache.ContainsKey((uintptr_t)Sender))
				{
					Tt->Remove(Sender);
					break;
				}
			}
		}
	}

	void ToolTip::WmShow()
	{
		RECT R{};
		GetWindowRect(this->_Handle, &R);

		TOOLINFOA Ti{};
		Ti.cbSize = sizeof(Ti);

		auto Result = (int32_t)SendMessageA(this->_Handle, TTM_GETCURRENTTOOLA, NULL, (LPARAM)&Ti);

		if (Result != NULL)
		{
			Drawing::Rectangle Rc(R.left, R.top, R.right - R.left, R.bottom - R.top);
			Drawing::Size CurrentToolTipSize;

			Rc.GetSize(&CurrentToolTipSize);

			auto Ctrl = Control::FromHandle(Ti.hwnd);

			if (Ctrl == nullptr)
				return;

			auto EventArgs = std::make_unique<PopupEventArgs>(Ctrl, Ctrl, IsBalloon(), CurrentToolTipSize);
			OnPopup(EventArgs);

			GetWindowRect(this->_Handle, &R);
			if (EventArgs->ToolTipSize.Equals(CurrentToolTipSize))
				Rc.GetSize(&CurrentToolTipSize);
			else
				CurrentToolTipSize = EventArgs->ToolTipSize;

			if (IsBalloon())
			{
				SendMessageA(this->_Handle, TTM_ADJUSTRECT, (WPARAM)1, (LPARAM)&R);

				if ((R.bottom - R.top) > CurrentToolTipSize.Height)
					CurrentToolTipSize.Height = (R.bottom - R.top);
			}

			if (!CurrentToolTipSize.Equals(Drawing::Size(R.right - R.left, R.bottom - R.top)))
			{
				auto CursorPos = Control::GetMousePosition();
				HMONITOR MonHandle = MonitorFromPoint({ CursorPos.X, CursorPos.Y }, MONITOR_DEFAULTTONEAREST);

				if (MonHandle != nullptr)
				{
					MONITORINFO Info;
					Info.cbSize = sizeof(Info);
					GetMonitorInfo(MonHandle, &Info);

					int32_t MaxWidth = 0;

					if (IsBalloon())
						MaxWidth = min(CurrentToolTipSize.Width - 2 * (int32_t)XBALLOONOFFSET, (Info.rcWork.right - Info.rcWork.left));
					else
						MaxWidth = min(CurrentToolTipSize.Width, (Info.rcWork.right - Info.rcWork.left));

					SendMessageA(this->_Handle, TTM_SETMAXTIPWIDTH, NULL, (LPARAM)MaxWidth);
				}
			}

			if (EventArgs->Cancel)
			{
				this->_Flags[9] = true;
				SetWindowPos(this->_Handle, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER);
			}
			else
			{
				this->_Flags[9] = false;
				SetWindowPos(this->_Handle, HWND_TOPMOST, R.left, R.top, CurrentToolTipSize.Width, CurrentToolTipSize.Height, SWP_NOACTIVATE | SWP_NOOWNERZORDER);
			}
		}
	}

	void ToolTip::WmPop()
	{
		TOOLINFOA Ti{};
		Ti.cbSize = sizeof(Ti);

		auto Result = (int32_t)SendMessageA(this->_Handle, TTM_GETCURRENTTOOLA, NULL, (LPARAM)&Ti);

		if (Result != 0)
		{
			auto Ctrl = Control::FromHandle(Ti.hwnd);
			if (Ctrl == nullptr)
				return;

			// This should only happen on Type::Auto / Type::SemiAbsolute

			auto CursorPos = Control::GetMousePosition();
			HMONITOR MonHandle = MonitorFromPoint({ CursorPos.X, CursorPos.Y }, MONITOR_DEFAULTTONEAREST);

			if (MonHandle != nullptr)
			{
				MONITORINFO Info;
				Info.cbSize = sizeof(Info);
				GetMonitorInfo(MonHandle, &Info);

				SendMessageA(this->_Handle, TTM_SETMAXTIPWIDTH, NULL, (Info.rcWork.right - Info.rcWork.left));
			}

			// TODO: If we support single-shot tooltips, remove them here...
		}
	}

	void ToolTip::WmMove()
	{
		RECT R{};
		GetWindowRect(this->_Handle, &R);

		TOOLINFOA Ti{};
		Ti.cbSize = sizeof(Ti);

		auto Result = (int32_t)SendMessageA(this->_Handle, TTM_GETCURRENTTOOLA, NULL, (LPARAM)&Ti);

		if (Result != 0)
		{
			auto Win = Control::FromHandle(Ti.hwnd);

			if (Win == nullptr)
				return;

			// TODO: If Info != Point::Empty:
			// TODO: Reposition the control...
		}
	}

	bool ToolTip::WmWindowPosChanged()
	{
		if (this->_Flags[9])
		{
			SendMessageA(this->_Handle, SW_HIDE, NULL, NULL);
			return true;
		}

		return false;
	}

	void ToolTip::WmWindowFromPoint(Message& Msg)
	{
		POINT* Pt = (POINT*)Msg.LParam;
		Drawing::Point ScreenCoords(Pt->x, Pt->y);
		bool Result = false;
		Msg.Result = GetWindowFromPoint(ScreenCoords, Result);
	}

	void ToolTip::AdjustBaseFromAuto()
	{
		_DelayTimes[TTDT_RESHOW] = _DelayTimes[TTDT_AUTOMATIC] / RESHOW_RATIO;
		_DelayTimes[TTDT_AUTOPOP] = _DelayTimes[TTDT_AUTOMATIC] / AUTOPOP_RATIO;
		_DelayTimes[TTDT_INITIAL] = _DelayTimes[TTDT_AUTOMATIC];
	}

	void ToolTip::SetDelayTime(uint32_t Type, uint32_t Time)
	{
		if (Type == TTDT_AUTOMATIC)
			this->_Flags[1] = true;
		else
			this->_Flags[1] = false;

		_DelayTimes[Type] = Time;

		if (GetState(ControlStates::StateCreated) && Time >= 0)
		{
			SendMessageA(this->_Handle, TTM_SETDELAYTIME, (WPARAM)Type, (LPARAM)Time);

			if (this->_Flags[1])
			{
				_DelayTimes[TTDT_AUTOPOP] = GetDelayTime(TTDT_AUTOPOP);
				_DelayTimes[TTDT_INITIAL] = GetDelayTime(TTDT_INITIAL);
				_DelayTimes[TTDT_RESHOW] = GetDelayTime(TTDT_RESHOW);
			}
		}
		else if (this->_Flags[1])
		{
			AdjustBaseFromAuto();
		}
	}

	uint32_t ToolTip::GetDelayTime(uint32_t Type)
	{
		if (GetState(ControlStates::StateCreated))
			return (uint32_t)SendMessageA(this->_Handle, TTM_GETDELAYTIME, (WPARAM)Type, NULL);
		else
			return _DelayTimes[Type];
	}

	uintptr_t ToolTip::GetWindowFromPoint(Drawing::Point ScreenCoords, bool& Success)
	{
		HWND BaseHwnd = NULL;
		Control* BaseVar = this->FindForm();
		
		if (BaseVar != nullptr)
			BaseHwnd = BaseVar->GetHandle();

		HWND hWnd = NULL;
		bool FinalMatch = false;

		while (!FinalMatch)
		{
			auto Pt = ScreenCoords;
			if (BaseVar != nullptr)
				Pt = BaseVar->PointToClient(ScreenCoords);

			auto Found = ChildWindowFromPointEx(BaseHwnd, { Pt.X, Pt.Y }, CWP_SKIPINVISIBLE);

			if (Found == BaseHwnd)
			{
				hWnd = Found;
				FinalMatch = true;
			}
			else if (Found == NULL)
			{
				FinalMatch = true;
			}
			else
			{
				BaseVar = Control::FromHandle(Found);
				if (BaseVar == nullptr)
				{
					BaseVar = Control::FromChildHandle(Found);
					if (BaseVar != nullptr)
						hWnd = BaseVar->GetHandle();

					FinalMatch = true;
				}
				else
					BaseHwnd = BaseVar->GetHandle();
			}
		}

		if (hWnd != NULL)
		{
			auto Ctrl = Control::FromHandle(hWnd);
			if (Ctrl != nullptr)
			{
				Control* Current = Ctrl;
				while (Current != nullptr && Current->Visible())
					Current = Current->Parent();

				if (Current != nullptr)
					hWnd = NULL;

				Success = true;
			}
		}

		return (uintptr_t)hWnd;
	}
}
