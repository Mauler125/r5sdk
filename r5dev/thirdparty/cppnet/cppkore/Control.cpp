#include "stdafx.h"
#include "Control.h"
#include "ContainerControl.h"

// System::Drawing*
#include "DrawingBase.h"
#include "BufferedGraphics.h"

namespace Forms
{
	// Custom message index cache
	uint32_t Control::WM_MOUSEENTER = 0;
	uint32_t Control::WM_INVOKEUI = 0;

	// GDI+ palette for rendering
	HPALETTE Control::HalftonePalette = nullptr;

	Control::Control()
		: _Handle(nullptr), _WndProcBase(NULL), _RTTI(ControlTypes::Control), _Parent(nullptr), _MinimumHeight(0), _MinimumWidth(0), _MaximumHeight(0), _MaximumWidth(0), _Anchor(AnchorStyles::Top | AnchorStyles::Left), _AnchorDeltas(), _Font(nullptr), _BackColor(Drawing::GetSystemColor(Drawing::SystemColors::Control)), _ForeColor(Drawing::GetSystemColor(Drawing::SystemColors::ControlText)), _BackColorBrush(0), _TabIndex(0),\
		_ControlStates((ControlStates)0), _ControlStyles((ControlStyles)0), _ClientWidth(0), _ClientHeight(0), _Width(0), _Height(0), _X(0), _Y(0), _RequiredScalingEnabled(false), _RequiredScaling(BoundsSpecified::All), _LayoutSuspendCount(0)
	{
		if (Control::WM_INVOKEUI == 0 || Control::WM_MOUSEENTER == 0)
		{
			Control::WM_MOUSEENTER = RegisterWindowMessageA("KoreMouseEnter");
			Control::WM_INVOKEUI = RegisterWindowMessageA("KoreUIInvoke");
		}

		SetState(ControlStates::StateVisible |
			ControlStates::StateEnabled |
			ControlStates::StateTabstop |
			ControlStates::StateCausesValidation, true);

		SetStyle(ControlStyles::AllPaintingInWmPaint |
			ControlStyles::UserPaint |
			ControlStyles::StandardClick |
			ControlStyles::StandardDoubleClick |
			ControlStyles::UseTextForAccessibility |
			ControlStyles::Selectable, true);
	}

	Control::~Control()
	{
		this->Dispose();
	}

	void Control::Show()
	{
		SetVisible(true);
	}

	void Control::Hide()
	{
		SetVisible(false);
	}

	void Control::WndProc(Message& Msg)
	{
		//
		// This is the base processor for every single control, we support all windows control events here
		//

		switch (Msg.Msg)
		{
		case WM_CLOSE:
			WmClose(Msg);
			break;
		case WM_MOVE:
			WmMove(Msg);
			break;
		case WM_WINDOWPOSCHANGED:
			WmWindowPosChanged(Msg);
			break;
		case WM_PARENTNOTIFY:
			WmParentNotify(Msg);
			break;
		case WM_ERASEBKGND:
			WmEraseBkgnd(Msg);
			break;
		case WM_COMMAND:
			WmCommand(Msg);
			break;
		case WM_SHOWWINDOW:
			WmShowWindow(Msg);
			break;
		case WM_PAINT:
			if (GetStyle(ControlStyles::UserPaint))
				WmPaint(Msg);
			else
				DefWndProc(Msg);
			break;
		case WM_LBUTTONDBLCLK:
			WmMouseDown(Msg, MouseButtons::Left, 2);
			if (GetStyle(ControlStyles::StandardDoubleClick))
				SetState(ControlStates::StateDoubleClickFired, true);
			break;
		case WM_LBUTTONDOWN:
			WmMouseDown(Msg, MouseButtons::Left, 1);
			break;
		case WM_LBUTTONUP:
			WmMouseUp(Msg, MouseButtons::Left, 1);
			break;
		case WM_RBUTTONDBLCLK:
			WmMouseDown(Msg, MouseButtons::Right, 2);
			if (GetStyle(ControlStyles::StandardDoubleClick))
				SetState(ControlStates::StateDoubleClickFired, true);
			break;
		case WM_RBUTTONDOWN:
			WmMouseDown(Msg, MouseButtons::Right, 1);
			break;
		case WM_RBUTTONUP:
			WmMouseUp(Msg, MouseButtons::Right, 1);
			break;
		case WM_MBUTTONDBLCLK:
			WmMouseDown(Msg, MouseButtons::Middle, 2);
			if (GetStyle(ControlStyles::StandardDoubleClick))
				SetState(ControlStates::StateDoubleClickFired, true);
			break;
		case WM_MBUTTONDOWN:
			WmMouseDown(Msg, MouseButtons::Middle, 1);
			break;
		case WM_MBUTTONUP:
			WmMouseUp(Msg, MouseButtons::Middle, 1);
			break;
		case WM_SETCURSOR:
			WmSetCursor(Msg);
			break;
		case WM_MOUSEMOVE:
			WmMouseMove(Msg);
			break;
		case WM_MOUSELEAVE:
			WmMouseLeave(Msg);
			break;
		case WM_MOUSEHOVER:
			WmMouseHover(Msg);
			break;
		case WM_CHAR:
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYUP:
			WmKeyChar(Msg);
			break;
		case WM_QUERYNEWPALETTE:
			WmQueryNewPalette(Msg);
			break;
		case WM_NOTIFY:
			WmNotify(Msg);
			break;
		case WM_NOTIFYFORMAT:
			WmNotifyFormat(Msg);
			break;
		case WM_CAPTURECHANGED:
			WmCaptureChanged(Msg);
			break;
		case WM_KILLFOCUS:
			WmKillFocus(Msg);
			break;
		case WM_SETFOCUS:
			WmSetFocus(Msg);
			break;
		case WM_MOUSEWHEEL:
			// TrackMouseEvent doesn't handle MouseWheel properly
			ResetMouseEventArgs();
			WmMouseWheel(Msg);
			break;

			// Handle all CTLCOLOR messages and reflected ones
		case WM_CTLCOLORBTN:
		case WM_CTLCOLORDLG:
		case WM_CTLCOLORMSGBOX:
		case WM_CTLCOLORSCROLLBAR:
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORLISTBOX:
		case WM_CTLCOLORSTATIC:
		case WM_REFLECT + WM_CTLCOLORBTN:
		case WM_REFLECT + WM_CTLCOLORDLG:
		case WM_REFLECT + WM_CTLCOLORMSGBOX:
		case WM_REFLECT + WM_CTLCOLORSCROLLBAR:
		case WM_REFLECT + WM_CTLCOLOREDIT:
		case WM_REFLECT + WM_CTLCOLORLISTBOX:
		case WM_REFLECT + WM_CTLCOLORSTATIC:
			WmCtlColorControl(Msg);
			break;

		default:

			//
			// Handle the events which aren't constant...
			//
			
			if (Msg.Msg == WM_MOUSEENTER)
			{
				WmMouseEnter(Msg);
				break;
			}
			else if (Msg.Msg == WM_INVOKEUI)
			{
				WmInvokeOnUIThread(Msg);
				break;
			}

			//dprintf("WinDBG: Unhandled WndProc: %d {0x%x} hWnd 0x%llx class 0x%llx\n", Msg, Msg, (intptr_t)hWnd, (uintptr_t)this);

			//
			// Default logic will proxy off the message to the proper base WndProc
			//

			DefWndProc(Msg);
			break;
		}
	}

	void Control::CreateControl(Control* Parent)
	{
		// Prevent duplicate calls to CreateControl()
		if (GetState(ControlStates::StateCreated))
			return;

		this->_Parent = Parent;

		// Get control params and register the class
		auto Cp = this->GetCreateParams();
		auto NeedsSubclass = false;
		auto CName = Control::RegisterWndClass((const char*)Cp.ClassName, Cp.ClassStyle, NeedsSubclass);

		// Create the control
		this->_Handle = CreateWindowExA(Cp.ExStyle, (const char*)CName, (const char*)Cp.Caption, Cp.Style, Cp.X, Cp.Y, Cp.Width, Cp.Height, (this->_Parent) ? this->_Parent->GetHandle() : NULL, NULL, GetModuleHandle(NULL), (LPVOID)this);

		// Ensure we have a class pointer reference
		this->_WndProcBase = (NeedsSubclass) ? SetWindowLongPtrA(this->_Handle, GWLP_WNDPROC, (intptr_t)&Control::InternalWndProc) : NULL;
		SetWindowLongPtrA(this->_Handle, GWLP_USERDATA, (intptr_t)this);

		// Setup the default font if none was previously set

		if (this->_Font == nullptr)
			if (this->_Parent && this->_Parent->_Font)
				this->_Font = std::make_unique<Drawing::Font>(this->_Handle, this->_Parent->_Font.get()->GetFontHandle());
			else
				this->_Font = std::make_unique<Drawing::Font>(this->_Handle, (HFONT)GetStockObject(DEFAULT_GUI_FONT));

		// Notify the window of the font selection
		SendMessageA(this->_Handle, WM_SETFONT, (WPARAM)this->_Font->GetFontHandle(), NULL);

		// Setup the state
		SetState(ControlStates::StateCreated, true);

		// Notify the handle was made, and is setup
		OnHandleCreated();

		// Update bounds
		UpdateBounds();
	}

	bool Control::Focus()
	{
		if (CanFocus())
			SetFocus(this->_Handle);

		return Focused();
	}

	bool Control::Focused()
	{
		return (this->_Handle != nullptr) && (GetFocus() == this->_Handle);
	}

	bool Control::Enabled()
	{
		if (!GetState(ControlStates::StateEnabled))
			return false;
		else if (this->_Parent == nullptr)
			return true;

		return this->_Parent->Enabled();
	}

	void Control::SetEnabled(bool Value)
	{
		bool oValue = Enabled();
		SetState(ControlStates::StateEnabled, Value);

		if (oValue != Value)
		{
			if (!Value)
				SelectNextIfFocused();

			OnEnabledChanged();
		}
	}

	bool Control::AllowDrop()
	{
		return GetState(ControlStates::StateAllowDrop);
	}

	void Control::SetAllowDrop(bool Value)
	{
		if (GetState(ControlStates::StateAllowDrop) != Value)
		{
			SetState(ControlStates::StateAllowDrop, Value);
			SetAcceptDrops(Value);
		}
	}

	bool Control::Visible()
	{
		return GetVisibleCore();
	}

	void Control::SetVisible(bool Value)
	{
		SetVisibleCore(Value);
	}

	bool Control::CanFocus()
	{
		if (this->_Handle == nullptr)
			return false;

		return (IsWindowVisible(this->_Handle)) && (IsWindowEnabled(this->_Handle));
	}

	bool Control::CanSelect()
	{
		if (!GetStyle(ControlStyles::Selectable))
			return false;

		for (Control* Ctl = this; Ctl != nullptr; Ctl = Ctl->_Parent)
		{
			if (!Ctl->Enabled() || !Ctl->Visible())
				return false;
		}

		return true;
	}

	bool Control::ContainsFocus()
	{
		if (!GetState(ControlStates::StateCreated))
			return false;

		auto FocusHwnd = GetFocus();

		if (FocusHwnd == NULL)
			return false;
		if (FocusHwnd == this->_Handle)
			return true;
		if (IsChild(this->_Handle, FocusHwnd))
			return true;

		return false;
	}

	void Control::SuspendLayout()
	{
		this->_LayoutSuspendCount++;

		if (this->_LayoutSuspendCount == 1)
		{
			// Not needed right now for dpi scaling...
			// TODO: OnLayoutSuspended();
		}
	}

	void Control::ResumeLayout(bool PerformLayout)
	{
		bool PerformedLayout = false;

		if (_LayoutSuspendCount > 0)
		{
			if (_LayoutSuspendCount == 1)
			{
				_LayoutSuspendCount++;

				try
				{
					OnLayoutResuming(PerformLayout);
					_LayoutSuspendCount--;
				}
				catch(...)
				{
					_LayoutSuspendCount--;
				}
			}

			_LayoutSuspendCount--;

			if (_LayoutSuspendCount == 0 && GetState(ControlStates::StateLayoutDeferred) && PerformLayout)
			{
				this->PerformLayout();
				PerformedLayout = true;
			}
		}
	}

	bool Control::DoubleBuffered()
	{
		return GetStyle(ControlStyles::OptimizedDoubleBuffer);
	}

	void Control::SetDoubleBuffered(bool Value)
	{
		SetStyle(ControlStyles::OptimizedDoubleBuffer, Value);
	}

	bool Control::Capture()
	{
		return GetState(ControlStates::StateCreated) && GetCapture() == this->_Handle;
	}

	void Control::SetCapture(bool Value)
	{
		if (Capture() == Value)
			return;

		if (Value)
			::SetCapture(this->_Handle);
		else
			::ReleaseCapture();
	}

	AnchorStyles Control::Anchor()
	{
		return _Anchor;
	}

	void Control::SetAnchor(AnchorStyles Value)
	{
		// Get the initial rectangle...
		this->UpdateInitialPos();

		// Apply the anchor value...
		this->_Anchor = Value;

		// Ensure delta is set...
		UpdateDeltas();

		// If we are a container control, enumerate children, call SetAnchor(Anchor());
		if (GetStyle(ControlStyles::ContainerControl) && this->_Controls != nullptr)
		{
			// Store the counts on the stack
			uint32_t ControlCount = this->_Controls->Count();
			auto& ControlList = *this->_Controls.get();

			for (uint32_t i = 0; i < ControlCount; i++)
			{
				auto Child = ControlList[i];
				Child->SetAnchor(Child->_Anchor);
			}
		}
	}

	uint32_t Control::TabIndex()
	{
		return this->_TabIndex;
	}

	void Control::SetTabIndex(uint32_t Value)
	{
		if (this->_TabIndex != Value)
		{
			this->_TabIndex = Value;
			// TODO: OnTabIndexChanged();
		}
	}

	Drawing::Point Control::Location()
	{
		return Drawing::Point(this->_X, this->_Y);
	}

	void Control::SetLocation(Drawing::Point Value)
	{
		SetBounds(Value.X, Value.Y, this->_Width, this->_Height);
		UpdateInitialPos();
	}

	Drawing::Size Control::Size()
	{
		return Drawing::Size(this->_Width, this->_Height);
	}

	void Control::SetSize(Drawing::Size Value)
	{
		SetBounds(this->_X, this->_Y, Value.Width, Value.Height);
		UpdateInitialPos();
	}

	Drawing::Size Control::MaximumSize()
	{
		return Drawing::Size(this->_MaximumWidth, this->_MaximumHeight);
	}

	void Control::SetMaximumSize(Drawing::Size Value)
	{
		this->_MaximumWidth = Value.Width;
		this->_MaximumHeight = Value.Height;

		// TODO: OnMaximumSizeChanged(); / Trigger reflow...
	}

	Drawing::Size Control::MinimumSize()
	{
		return Drawing::Size(this->_MinimumWidth, this->_MinimumHeight);
	}

	void Control::SetMinimumSize(Drawing::Size Value)
	{
		this->_MinimumWidth = Value.Width;
		this->_MinimumHeight = Value.Height;

		// TODO: OnMinimumSizeChanged();
	}

	Drawing::Color Control::BackColor()
	{
		return this->_BackColor;
	}

	void Control::SetBackColor(Drawing::Color Color)
	{
		this->_BackColor = Color;
		OnBackColorChanged();
	}

	Drawing::Color Control::ForeColor()
	{
		return this->_ForeColor;
	}

	void Control::SetForeColor(Drawing::Color Color)
	{
		this->_ForeColor = Color;
		OnForeColorChanged();
	}

	Drawing::Font* Control::GetFont()
	{
		if (this->_Font != nullptr)
			return this->_Font.get();

		// Set to system wide default fault
		this->_Font = std::make_unique<Drawing::Font>(this->_Handle, (HFONT)GetStockObject(DEFAULT_GUI_FONT));

		return this->_Font.get();
	}

	void Control::SetFont(Drawing::Font* Font)
	{
		this->_Font.reset(Font);
		OnFontChanged();
	}

	Control* Control::Parent()
	{
		return this->_Parent;
	}

	void Control::SetParent(Control* Value)
	{
		_Parent = Value;

		if (_Parent && _Parent->GetState(ControlStates::StateCreated))
			::SetParent(this->_Handle, _Parent->_Handle);
	}

	Drawing::Rectangle Control::ClientRectangle()
	{
		return Drawing::Rectangle(0, 0, this->_ClientWidth, this->_ClientHeight);
	}

	Drawing::Size Control::ClientSize()
	{
		return Drawing::Size(this->_ClientWidth, this->_ClientHeight);
	}

	void Control::SetClientSize(Drawing::Size Value)
	{
		SetSize(SizeFromClientSize(Value.Width, Value.Height));
		_ClientWidth = Value.Width;
		_ClientHeight = Value.Height;
		OnClientSizeChanged();
	}

	Drawing::Point Control::PointToScreen(const Drawing::Point& Point)
	{
		POINT Pt;
		Pt.x = Point.X;
		Pt.y = Point.Y;

		MapWindowPoints(this->_Handle, NULL, &Pt, 1);

		return Drawing::Point(Pt.x, Pt.y);
	}

	Drawing::Point Control::PointToClient(const Drawing::Point& Point)
	{
		POINT Pt;
		Pt.x = Point.X;
		Pt.y = Point.Y;

		MapWindowPoints(NULL, this->_Handle, &Pt, 1);

		return Drawing::Point(Pt.x, Pt.y);
	}

	Drawing::Rectangle Control::RectangleToScreen(const Drawing::Rectangle& Rect)
	{
		RECT Rc;
		Rc.left = Rect.X;
		Rc.top = Rect.Y;
		Rc.right = (Rect.X + Rect.Width);
		Rc.bottom = (Rect.Y + Rect.Height);

		MapWindowPoints(this->_Handle, NULL, (LPPOINT)&Rc, 2);

		return Drawing::Rectangle(Rc.left, Rc.top, (Rc.right - Rc.left), (Rc.bottom - Rc.top));
	}

	Drawing::Rectangle Control::RectangleToClient(const Drawing::Rectangle & Rect)
	{
		RECT Rc;
		Rc.left = Rect.X;
		Rc.top = Rect.Y;
		Rc.right = (Rect.X + Rect.Width);
		Rc.bottom = (Rect.Y + Rect.Height);

		MapWindowPoints(NULL, this->_Handle, (LPPOINT)&Rc, 2);

		return Drawing::Rectangle(Rc.left, Rc.top, (Rc.right - Rc.left), (Rc.bottom - Rc.top));
	}

	String Control::Text()
	{
		return this->WindowText();
	}

	void Control::SetText(const String& Value)
	{
		this->SetWindowText(Value);
		OnTextChanged();
	}

	void Control::BringToFront()
	{
		if (_Parent != nullptr)
			_Parent->_Controls->SetChildIndex(this, 0);
		else if (GetState(ControlStates::StateCreated) && GetState(ControlStates::StateTopLevel) && IsWindowEnabled(this->_Handle))
			SetWindowPos(this->_Handle, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}

	void Control::SendToBack()
	{
		if (_Parent != nullptr)
			_Parent->_Controls->SetChildIndex(this, -1);
		else if (GetState(ControlStates::StateCreated) && GetState(ControlStates::StateTopLevel))
			SetWindowPos(this->_Handle, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}

	bool Control::GetStyle(ControlStyles Flag)
	{
		return ((int)this->_ControlStyles & (int)Flag) == (int)Flag;
	}

	void Control::SetStyle(ControlStyles Flags, bool Value)
	{
		this->_ControlStyles = Value ? (ControlStyles)((int)this->_ControlStyles | (int)Flags) : (ControlStyles)((int)this->_ControlStyles & ~(int)Flags);
	}

	bool Control::GetState(ControlStates Flag)
	{
		return ((int)this->_ControlStates & (int)Flag) == (int)Flag;
	}

	void Control::SetState(ControlStates Flags, bool Value)
	{
		this->_ControlStates = Value ? (ControlStates)((int)this->_ControlStates | (int)Flags) : (ControlStates)((int)this->_ControlStates & ~(int)Flags);
	}

	void Control::Invalidate(bool InvalidateChildren)
	{
		if (GetState(ControlStates::StateCreated))
		{
			if (InvalidateChildren)
				RedrawWindow(this->_Handle, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
			else
				InvalidateRect(this->_Handle, nullptr, !GetStyle(ControlStyles::Opaque));

			OnInvalidated(std::make_unique<InvalidateEventArgs>(this->ClientRectangle()));
		}
	}

	void Control::Invoke(Action Method)
	{
		SendMessageA(this->_Handle, WM_INVOKEUI, NULL, (LPARAM)Method);
	}

	bool Control::InvokeRequired()
	{
		DWORD Pid;
		DWORD HwndThread = GetWindowThreadProcessId(this->_Handle, &Pid);
		DWORD CurrentThread = GetCurrentThreadId();

		return (HwndThread != CurrentThread);
	}

	void Control::Update()
	{
		UpdateWindow(this->_Handle);
	}

	void Control::Refresh()
	{
		Invalidate(true);
		Update();
	}

	HWND Control::GetHandle()
	{
		return this->_Handle;
	}

	ControlTypes Control::GetType()
	{
		return this->_RTTI;
	}

	uint32_t Control::GetControlCount()
	{
		return this->_Controls->Count();
	}

	Control* Control::FindForm()
	{
		auto Cur = this;

		while (Cur != nullptr && (Cur->_RTTI != ControlTypes::Form))
			Cur = Cur->_Parent;

		return Cur;
	}

	Control* Control::GetContainerControl()
	{
		Control* C = this;

		while (C != nullptr && (!IsFocusManagingContainerControl(C)))
			C = C->_Parent;

		return C;
	}

	Control* Control::GetNextControl(Control* Ctrl, bool Forward)
	{
		if (!Contains(Ctrl))
			Ctrl = this;

		if (Forward)
		{
			if (this->_Controls != nullptr && this->_Controls->Count() > 0 && (Ctrl == this || !IsFocusManagingContainerControl(Ctrl)))
			{
				auto Found = Ctrl->GetFirstChildcontrolInTabOrder(true);
				if (Found != nullptr)
					return Found;
			}

			while (Ctrl != this)
			{
				uint32_t TargetIndex = Ctrl->_TabIndex;
				bool HitCtrl = false;
				Control* Found = nullptr;
				Control* P = Ctrl->_Parent;

				uint32_t ParentControlCount = 0;

				auto& ParentControls = *P->_Controls.get();

				if (P->_Controls != nullptr)
					ParentControlCount = ParentControls.Count();

				for (uint32_t i = 0; i < ParentControlCount; i++)
				{
					if (ParentControls[i] != Ctrl)
					{
						if (ParentControls[i]->_TabIndex >= TargetIndex)
						{
							if (Found == nullptr || Found->_TabIndex > ParentControls[i]->_TabIndex)
							{
								if (ParentControls[i]->_TabIndex != TargetIndex || HitCtrl)
									Found = ParentControls[i];
							}
						}
					}
					else
					{
						HitCtrl = true;
					}
				}

				if (Found != nullptr)
					return Found;

				Ctrl = Ctrl->_Parent;
			}
		}
		else
		{
			if (Ctrl != this)
			{
				uint32_t TargetIndex = Ctrl->_TabIndex;
				bool HitCtrl = false;
				Control* Found = nullptr;
				Control* P = Ctrl->_Parent;

				uint32_t ParentControlCount = 0;

				auto& ParentControls = *P->_Controls.get();

				if (P->_Controls != nullptr)
					ParentControlCount = ParentControls.Count();

				for (int32_t i = (int32_t)ParentControlCount - 1; i >= 0; i--)
				{
					if (ParentControls[i] != Ctrl)
					{
						if (ParentControls[i]->_TabIndex <= TargetIndex)
						{
							if (Found == nullptr || Found->_TabIndex < ParentControls[i]->_TabIndex)
							{
								if (ParentControls[i]->_TabIndex != TargetIndex || HitCtrl)
									Found = ParentControls[i];
							}
						}
					}
					else
					{
						HitCtrl = true;
					}
				}

				if (Found != nullptr)
					Ctrl = Found;
				else
				{
					if (P == this)
						return nullptr;
					else
						return P;
				}
			}

			auto CtrlControls = this->_Controls.get();

			while (CtrlControls != nullptr && CtrlControls->Count() > 0 && (Ctrl == this || !IsFocusManagingContainerControl(Ctrl)))
			{
				auto Found = Ctrl->GetFirstChildcontrolInTabOrder(false);
				if (Found != nullptr)
				{
					Ctrl = Found;
					CtrlControls = Ctrl->_Controls.get();
				}
				else
				{
					break;
				}
			}
		}

		return (Ctrl == this) ? nullptr : Ctrl;
	}

	bool Control::Contains(Control* Ctrl)
	{
		while (Ctrl != nullptr)
		{
			Ctrl = Ctrl->_Parent;
			if (Ctrl == nullptr)
				return false;
			if (Ctrl == this)
				return true;
		}

		return false;
	}

	const std::unique_ptr<ControlCollection>& Control::Controls()
	{
		return this->_Controls;
	}

	void Control::Select()
	{
		Select(false, false);
	}

	bool Control::SelectNextControl(Control* Ctrl, bool Forward, bool TabStopOnly, bool Nested, bool Wrap)
	{
		auto NextCtrl = this->GetNextSelectableControl(Ctrl, Forward, TabStopOnly, Nested, Wrap);
		if (NextCtrl != nullptr)
		{
			NextCtrl->Select(true, Forward);
			return true;
		}
		
		return false;
	}

	void Control::UpdateZOrder()
	{
		if (_Parent != nullptr)
			_Parent->UpdateChildZOrder(this);
	}

	void Control::DefWndProc(Message& Msg)
	{
		// We must properly proxy off the WndProc if we have a base...

		if (this->_WndProcBase != NULL)
			Msg.Result = CallWindowProcA((WNDPROC)this->_WndProcBase, (HWND)Msg.HWnd, Msg.Msg, Msg.WParam, Msg.LParam);
		else
			Msg.Result = DefWindowProcA((HWND)Msg.HWnd, Msg.Msg, Msg.WParam, Msg.LParam);
	}

	void Control::OnPaint(const std::unique_ptr<PaintEventArgs>& EventArgs)
	{
		Paint.RaiseEvent(EventArgs, this);
	}

	void Control::OnPaintBackground(const std::unique_ptr<PaintEventArgs>& EventArgs)
	{
		// Fill the back color of this control
		if (this->BackColor().GetA() == 255)
		{
			auto DC = (EventArgs->NativeHandle() != nullptr) ? EventArgs->NativeHandle() : EventArgs->Graphics->GetHDC();

			RECT Rc{ EventArgs->ClipRectangle.GetLeft(), EventArgs->ClipRectangle.GetTop(), EventArgs->ClipRectangle.GetRight(), EventArgs->ClipRectangle.GetBottom() };
			FillRect(DC, &Rc, (HBRUSH)this->BackColorBrush());

			if (EventArgs->NativeHandle() == nullptr)
				EventArgs->Graphics->ReleaseHDC(DC);
		}
		else
		{
			auto t = Drawing::SolidBrush(this->BackColor());
			EventArgs->Graphics->FillRectangle(&t, EventArgs->ClipRectangle);
		}
	}

	void Control::OnEnabledChanged()
	{
		if (this->_Handle != nullptr)
		{
			EnableWindow(this->_Handle, this->Enabled());

			// User-paint controls should repaint when their enabled state changes
			if (GetStyle(ControlStyles::UserPaint))
			{
				Invalidate();
				Update();
			}
		}

		EnabledChanged.RaiseEvent(this);
	}

	void Control::OnClick()
	{
		Click.RaiseEvent(this);
	}

	void Control::OnDoubleClick()
	{
		DoubleClick.RaiseEvent(this);
	}

	void Control::OnMouseClick(const std::unique_ptr<MouseEventArgs>& EventArgs)
	{
		MouseClick.RaiseEvent(EventArgs, this);
	}

	void Control::OnMouseDoubleClick(const std::unique_ptr<MouseEventArgs>& EventArgs)
	{
		MouseDoubleClick.RaiseEvent(EventArgs, this);
	}

	void Control::OnMouseUp(const std::unique_ptr<MouseEventArgs>& EventArgs)
	{
		MouseUp.RaiseEvent(EventArgs, this);
	}

	void Control::OnMouseDown(const std::unique_ptr<MouseEventArgs>& EventArgs)
	{
		MouseDown.RaiseEvent(EventArgs, this);
	}

	void Control::OnMouseMove(const std::unique_ptr<MouseEventArgs>& EventArgs)
	{
		MouseMove.RaiseEvent(EventArgs, this);
	}

	void Control::OnInvalidated(const std::unique_ptr<InvalidateEventArgs>& EventArgs)
	{
		Invalidated.RaiseEvent(EventArgs, this);
	}

	void Control::OnMouseWheel(const std::unique_ptr<HandledMouseEventArgs>& EventArgs)
	{
		MouseWheel.RaiseEvent(EventArgs, this);
	}

	void Control::OnKeyPress(const std::unique_ptr<KeyPressEventArgs>& EventArgs)
	{
		KeyPress.RaiseEvent(EventArgs, this);
	}

	void Control::OnKeyUp(const std::unique_ptr<KeyEventArgs>& EventArgs)
	{
		KeyUp.RaiseEvent(EventArgs, this);
	}

	void Control::OnKeyDown(const std::unique_ptr<KeyEventArgs>& EventArgs)
	{
		KeyDown.RaiseEvent(EventArgs, this);
	}

	void Control::OnDragEnter(const std::unique_ptr<DragEventArgs>& EventArgs)
	{
		DragEnter.RaiseEvent(EventArgs, this);
	}

	void Control::OnDragOver(const std::unique_ptr<DragEventArgs>& EventArgs)
	{
		DragOver.RaiseEvent(EventArgs, this);
	}

	void Control::OnDragDrop(const std::unique_ptr<DragEventArgs>& EventArgs)
	{
		DragDrop.RaiseEvent(EventArgs, this);
	}

	void Control::OnDragLeave()
	{
		DragLeave.RaiseEvent(this);
	}

	void Control::OnFontChanged()
	{
		Invalidate();

		if (GetState(ControlStates::StateCreated))
			SendMessageA(this->_Handle, WM_SETFONT, (WPARAM)this->_Font->GetFontHandle(), NULL);

		FontChanged.RaiseEvent(this);
	}

	void Control::OnVisibleChanged()
	{
		if (this->Visible())
			SetState(ControlStates::StateTrackingMouseEvent, false);

		VisibleChanged.RaiseEvent(this);

		// TODO: If we are a contianer control, we must enumerate children and call
		// OnParentVisibleChanged();
		// https://referencesource.microsoft.com/#System.Windows.Forms/winforms/Managed/System/WinForms/Control.cs,8578
	}

	void Control::OnHandleCreated()
	{
		// Restore drag drop ability
		SetAcceptDrops(AllowDrop());

		// We must create the controls here if we are a container control
		if (GetStyle(ControlStyles::ContainerControl) && this->_Controls != nullptr)
		{
			// Store the counts on the stack
			uint32_t ControlCount = this->_Controls->Count();
			auto& ControlList = *this->_Controls.get();

			for (uint32_t i = 0; i < ControlCount; i++)
			{
				if (!ControlList[i]->GetState(ControlStates::StateCreated))
					ControlList[i]->CreateControl(this);
			}

			SetAnchor(this->_Anchor);
		}

		HandleCreated.RaiseEvent(this);
	}

	void Control::OnHandleDestroyed()
	{
		HandleDestroyed.RaiseEvent(this);
	}

	void Control::OnTextChanged()
	{
		TextChanged.RaiseEvent(this);
	}

	void Control::OnMouseEnter()
	{
		MouseEnter.RaiseEvent(this);
	}

	void Control::OnMouseLeave()
	{
		MouseLeave.RaiseEvent(this);
	}

	void Control::OnMouseHover()
	{
		MouseHover.RaiseEvent(this);
	}

	void Control::OnLostFocus()
	{
		LostFocus.RaiseEvent(this);
	}

	void Control::OnGotFocus()
	{
		GotFocus.RaiseEvent(this);
	}

	void Control::OnStyleChanged()
	{
		StyleChanged.RaiseEvent(this);
	}

	void Control::OnLocationChanged()
	{
		LocationChanged.RaiseEvent(this);
	}

	void Control::OnSizeChanged()
	{
		OnResize();

		SizeChanged.RaiseEvent(this);
	}

	void Control::OnResize()
	{
		if (GetStyle(ControlStyles::ResizeRedraw))
			Invalidate();

		PerformLayout();

		Resize.RaiseEvent(this);
	}

	void Control::OnClientSizeChanged()
	{
		ClientSizeChanged.RaiseEvent(this);
	}

	void Control::OnMouseCaptureChanged()
	{
		MouseCaptureChanged.RaiseEvent(this);
	}

	void Control::OnBackColorChanged()
	{
		if (_BackColorBrush != (uintptr_t)0)
		{
			if (GetState(ControlStates::StateOwnCtlBrush))
				DeleteObject((HGDIOBJ)_BackColorBrush);

			_BackColorBrush = (uintptr_t)0;
		}

		Invalidate();
		BackColorChanged.RaiseEvent(this);
	}

	void Control::OnForeColorChanged()
	{
		Invalidate();
		ForeColorChanged.RaiseEvent(this);
	}

	HPALETTE Control::SetUpPalette(HDC Dc, bool Force, bool RealizePalette)
	{
		if (Control::HalftonePalette == nullptr)
			Control::HalftonePalette = Gdiplus::Graphics::GetHalftonePalette();

		auto Result = SelectPalette(Dc, Control::HalftonePalette, (Force ? FALSE : TRUE));

		if (Result != nullptr && RealizePalette)
			::RealizePalette(Dc);

		return Result;
	}

	MouseButtons Control::GetMouseButtons()
	{
		auto Result = (MouseButtons)0;

		if (GetKeyState((int)Keys::LButton) < 0)
			Result = Result | MouseButtons::Left;
		if (GetKeyState((int)Keys::RButton) < 0)
			Result = Result | MouseButtons::Right;
		if (GetKeyState((int)Keys::MButton) < 0)
			Result = Result | MouseButtons::Middle;
		if (GetKeyState((int)Keys::XButton1) < 0)
			Result = Result | MouseButtons::XButton1;
		if (GetKeyState((int)Keys::XButton2) < 0)
			Result = Result | MouseButtons::XButton2;

		return Result;
	}

	Drawing::Point Control::GetMousePosition()
	{
		POINT Pt{};
		GetCursorPos(&Pt);

		return Drawing::Point(Pt.x, Pt.y);
	}

	Keys Control::GetModifierKeys()
	{
		uint32_t Result = 0;

		if (GetKeyState((int)Keys::ShiftKey) < 0)
			Result |= (int)Keys::Shift;
		if (GetKeyState((int)Keys::ControlKey) < 0)
			Result |= (int)Keys::Control;
		if (GetKeyState((int)Keys::Menu) < 0)
			Result |= (int)Keys::Alt;

		return (Keys)Result;
	}

	void Control::UpdateBounds()
	{
		RECT Rc{};
		GetClientRect(this->_Handle, &Rc);

		if (!this->_AnchorDeltas.InitialRectSet && GetState(ControlStates::StateTopLevel))
		{
			std::memcpy(&this->_AnchorDeltas.InitialRect, &Rc, sizeof(Rc));
			this->_AnchorDeltas.InitialRectSet = true;
		}

		auto ClientWidth = Rc.right;
		auto ClientHeight = Rc.bottom;

		GetWindowRect(this->_Handle, &Rc);

		UpdateDeltas();

		if (!GetState(ControlStates::StateTopLevel))
			MapWindowPoints(NULL, GetParent(this->_Handle), (LPPOINT)&Rc, 2);

		if (!this->_AnchorDeltas.InitialRectSet && !GetState(ControlStates::StateTopLevel))
		{
			std::memcpy(&this->_AnchorDeltas.InitialRect, &Rc, sizeof(Rc));
			this->_AnchorDeltas.InitialRectSet = true;
		}

		UpdateBounds(Rc.left, Rc.top, Rc.right - Rc.left, Rc.bottom - Rc.top, ClientWidth, ClientHeight);
	}

	void Control::UpdateBounds(uint32_t X, uint32_t Y, uint32_t Width, uint32_t Height, uint32_t ClientWidth, uint32_t ClientHeight)
	{
		bool nLocation = this->_X != X || this->_Y != Y;
		bool nSize = this->_Width != Width || this->_Height != Height ||
			this->_ClientWidth != ClientWidth || this->_ClientHeight != ClientHeight;

		this->_X = X;
		this->_Y = Y;
		this->_Width = Width;
		this->_Height = Height;
		this->_ClientWidth = ClientWidth;
		this->_ClientHeight = ClientHeight;

		if (nLocation)
			OnLocationChanged();

		if (nSize)
		{
			OnSizeChanged();
			OnClientSizeChanged();
		}
	}

	void Control::UpdateBounds(uint32_t X, uint32_t Y, uint32_t Width, uint32_t Height)
	{
		RECT Rect{};

		auto Cp = this->GetCreateParams();

		AdjustWindowRectEx(&Rect, Cp.Style, false, Cp.ExStyle);

		int ClientWidth = Width - (Rect.right - Rect.left);
		int ClientHeight = Height - (Rect.bottom - Rect.top);

		UpdateBounds(X, Y, Width, Height, ClientWidth, ClientHeight);
	}

	void Control::SetBounds(uint32_t X, uint32_t Y, uint32_t Width, uint32_t Height)
	{
		if (!GetState(ControlStates::StateCreated))
			UpdateBounds(X, Y, Width, Height);
		else
		{
			if (!GetState(ControlStates::StateSizeLockedByOS))
			{
				auto Flags = SWP_NOZORDER | SWP_NOACTIVATE;

				if (this->_X == X && this->_Y==Y)
					Flags |= SWP_NOMOVE;
				if (this->_Width == Width && this->_Height == Height)
					Flags |= SWP_NOSIZE;

				::SetWindowPos(this->_Handle, NULL, X, Y, Width, Height, Flags);
			}
		}
	}

	void Control::UpdateDeltas()
	{
		bool AnchorLeft = ((int)this->_Anchor & (int)AnchorStyles::Left) == (int)AnchorStyles::Left;
		bool AnchorRight = ((int)this->_Anchor & (int)AnchorStyles::Right) == (int)AnchorStyles::Right;
		bool AnchorTop = ((int)this->_Anchor & (int)AnchorStyles::Top) == (int)AnchorStyles::Top;
		bool AnchorBottom = ((int)this->_Anchor & (int)AnchorStyles::Bottom) == (int)AnchorStyles::Bottom;

		if (AnchorLeft && AnchorRight)
		{
			this->_AnchorDeltas.XMoveFrac = 0.f;
			this->_AnchorDeltas.XSizeFrac = 1.f;
		}
		else if (AnchorLeft)
		{
			this->_AnchorDeltas.XMoveFrac = 0.f;
			this->_AnchorDeltas.XSizeFrac = 0.f;
		}
		else if (AnchorRight)
		{
			this->_AnchorDeltas.XMoveFrac = 1.f;
			this->_AnchorDeltas.XSizeFrac = 0.f;
		}
		else
		{
			this->_AnchorDeltas.XMoveFrac = 0.5f;
			this->_AnchorDeltas.XSizeFrac = 0.f;
		}

		if (AnchorTop && AnchorBottom)
		{
			this->_AnchorDeltas.YMoveFrac = 0.f;
			this->_AnchorDeltas.YSizeFrac = 1.f;
		}
		else if (AnchorTop)
		{
			this->_AnchorDeltas.YMoveFrac = 0.f;
			this->_AnchorDeltas.YSizeFrac = 0.f;
		}
		else if (AnchorBottom)
		{
			this->_AnchorDeltas.YMoveFrac = 1.f;
			this->_AnchorDeltas.YSizeFrac = 0.f;
		}
		else
		{
			this->_AnchorDeltas.YMoveFrac = 0.5f;
			this->_AnchorDeltas.YSizeFrac = 0.f;
		}
	}

	void Control::UpdateInitialPos()
	{
		// Get the initial rectangle...
		if (GetState(ControlStates::StateTopLevel))
		{
			::GetClientRect(this->_Handle, &this->_AnchorDeltas.InitialRect);
		}
		else
		{
			::GetWindowRect(this->_Handle, &this->_AnchorDeltas.InitialRect);
			::MapWindowPoints(NULL, GetParent(this->_Handle), (LPPOINT)&this->_AnchorDeltas.InitialRect, 2);
		}
	}

	void Control::Select(bool Directed, bool Forward)
	{
		auto C = (ContainerControl*)GetContainerControl();

		if (C != nullptr)
			C->SetActiveControl(this);
	}

	void Control::Scale(Drawing::SizeF IncludedFactor, Drawing::SizeF ExcludedFactor, Control* Ctrl)
	{
		ScaleControl(IncludedFactor, ExcludedFactor, Ctrl);
		ScaleChildControls(IncludedFactor, ExcludedFactor, Ctrl);
		PerformLayout();
	}

	void Control::OnLayoutResuming(bool PerformLayout)
	{
		if (this->_Parent != nullptr)
		{
			this->_Parent->OnChildLayoutResuming(this, PerformLayout);
		}
	}

	void Control::OnChildLayoutResuming(Control* Child, bool PerformLayout)
	{
		if (this->_Parent != nullptr)
		{
			this->_Parent->OnChildLayoutResuming(Child, PerformLayout);
		}
	}

	uint32_t Control::WindowStyle()
	{
		return GetWindowLong(this->_Handle, GWL_STYLE);
	}

	void Control::SetWindowStyle(uint32_t Value)
	{
		SetWindowLong(this->_Handle, GWL_STYLE, Value);
	}

	uint32_t Control::WindowExStyle()
	{
		return GetWindowLong(this->_Handle, GWL_EXSTYLE);
	}

	void Control::SetWindowExStyle(uint32_t Value)
	{
		SetWindowLong(this->_Handle, GWL_EXSTYLE, Value);
	}

	Drawing::Size Control::SizeFromClientSize(int32_t Width, int32_t Height)
	{
		RECT Rc{ 0, 0, Width, Height };
		auto Cp = GetCreateParams();

		AdjustWindowRectEx(&Rc, Cp.Style, FALSE, Cp.ExStyle);

		return Drawing::Size(Rc.right - Rc.left, Rc.bottom - Rc.top);
	}

	Drawing::Size Control::ScaleSize(Drawing::Size Start, float X, float Y)
	{
		Drawing::Size Result = Start;

		if (!GetStyle(ControlStyles::FixedWidth))
		{
			Result.Width = (int)std::roundf((float)Result.Width * X);
		}
		if (!GetStyle(ControlStyles::FixedHeight))
		{
			Result.Height = (int)std::roundf((float)Result.Height * Y);
		}

		return Result;
	}

	void Control::ScaleControl(Drawing::SizeF IncludedFactor, Drawing::SizeF ExcludedFactor, Control* Ctrl)
	{
		auto IncludedSpecified = BoundsSpecified::None;
		auto ExcludedSpecified = BoundsSpecified::None;

		if (!IncludedFactor.Empty())
		{
			IncludedSpecified = this->_RequiredScaling;
		}

		if (!ExcludedFactor.Empty())
		{
			ExcludedSpecified = (BoundsSpecified)(~(uint32_t)this->_RequiredScaling & (uint32_t)BoundsSpecified::All);
		}

		if (IncludedSpecified != BoundsSpecified::None)
		{
			ScaleControl(IncludedFactor, IncludedSpecified);
		}

		if (ExcludedSpecified != BoundsSpecified::None)
		{
			ScaleControl(ExcludedFactor, ExcludedSpecified);
		}

		if (!IncludedFactor.Empty())
		{
			this->_RequiredScaling = BoundsSpecified::None;
		}
	}

	void Control::ScaleChildControls(Drawing::SizeF IncludedFactor, Drawing::SizeF ExcludedFactor, Control* Ctrl)
	{
		// Must be container
		if (this->_Controls == nullptr)
			return;

		// Store the counts on the stack
		uint32_t ControlCount = this->_Controls->Count();
		auto& ControlList = *this->_Controls.get();

		for (uint32_t i = 0; i < ControlCount; i++)
			ControlList[i]->Scale(IncludedFactor, ExcludedFactor, Ctrl);
	}

	void Control::ScaleControl(Drawing::SizeF Factor, BoundsSpecified Specified)
	{
		auto Cp = this->GetCreateParams();
		RECT Adornments{};
		AdjustWindowRectEx(&Adornments, Cp.Style, FALSE, Cp.ExStyle);
		
		auto MinSize = this->MinimumSize();
		auto MaxSize = this->MaximumSize();

		this->SetMinimumSize({ 0, 0 });
		this->SetMaximumSize({ 0, 0 });

		auto ScaledBounds = this->GetScaledBounds(Drawing::Rectangle(this->Location(), this->Size()), Factor, Specified);

		auto AdornmentSize = Drawing::Size(Adornments.right - Adornments.left, Adornments.bottom - Adornments.top);
		if (!MinSize.Empty())
		{
			MinSize = MinSize - AdornmentSize;
			MinSize = this->ScaleSize(Drawing::UnionSizes(Drawing::Size{}, MinSize), Factor.Width, Factor.Height) + AdornmentSize;
		}
		if (!MaxSize.Empty())
		{
			MaxSize = MaxSize - AdornmentSize;
			MaxSize = this->ScaleSize(Drawing::UnionSizes(Drawing::Size{}, MaxSize), Factor.Width, Factor.Height) + AdornmentSize;
		}

		auto FMaxSize = Drawing::ConvertZeroToUnbounded(MaxSize);
		auto ScaledSize = Drawing::IntersectSizes({ ScaledBounds.Width, ScaledBounds.Height }, FMaxSize);
		ScaledSize = Drawing::UnionSizes(ScaledSize, MinSize);

		this->SetBounds(ScaledBounds.X, ScaledBounds.Y, ScaledSize.Width, ScaledSize.Height);

		this->SetMinimumSize(MinSize);
		this->SetMaximumSize(MaxSize);
	}

	Drawing::Rectangle Control::GetScaledBounds(Drawing::Rectangle Bounds, Drawing::SizeF Factor, BoundsSpecified Specified)
	{
		// We should not include the window adornments in our calculation,
		// because windows scales them for us.
		RECT Adornments{};
		auto Cp = this->GetCreateParams();
		AdjustWindowRectEx(&Adornments, Cp.Style, FALSE, Cp.ExStyle);

		float Dx = Factor.Width;
		float Dy = Factor.Height;

		int32_t Sx = Bounds.X;
		int32_t Sy = Bounds.Y;

		// Don't reposition top level controls.  Also, if we're in
		// design mode, don't reposition the root component.
		bool ScaleLoc = !GetState(ControlStates::StateTopLevel);

		if (ScaleLoc)
		{
			if (((uint32_t)Specified & (uint32_t)BoundsSpecified::X) != 0)
			{
				Sx = (int)std::roundf(Bounds.X * Dx);
			}

			if (((uint32_t)Specified & (uint32_t)BoundsSpecified::Y) != 0)
			{
				Sy = (int)std::roundf(Bounds.Y * Dy);
			}
		}

		int32_t Sw = Bounds.Width;
		int32_t Sh = Bounds.Height;

		if (!GetStyle(ControlStyles::FixedWidth) && ((uint32_t)Specified & (uint32_t)BoundsSpecified::Width) != 0)
		{
			auto adornmentWidth = (Adornments.right - Adornments.left);
			auto localWidth = Bounds.Width - adornmentWidth;
			Sw = (int)std::roundf(localWidth * Dx) + adornmentWidth;
		}
		if (!GetStyle(ControlStyles::FixedHeight) && ((uint32_t)Specified & (uint32_t)BoundsSpecified::Height) != 0)
		{
			auto adornmentHeight = (Adornments.bottom - Adornments.top);
			auto localHeight = Bounds.Height - adornmentHeight;
			Sh = (int)std::roundf(localHeight * Dy) + adornmentHeight;
		}

		return Drawing::Rectangle(Sx, Sy, Sw, Sh);
	}

	uintptr_t Control::BackColorBrush()
	{
		if (_BackColorBrush != (uintptr_t)0)
			return _BackColorBrush;

		if (_Parent != nullptr && _Parent->BackColor().ToCOLORREF() == BackColor().ToCOLORREF())
			return _Parent->BackColorBrush();

		auto Color = this->BackColor();
		auto ColorWin = Drawing::ColorToWin32(Color);

		// Convert the color to a system color if possible...
		int i = 0;
		for (; i <= 30; i++)
		{
			auto SysColor = GetSysColor(i);
			if (SysColor == ColorWin)
				break;
		}

		if (i <= 30)
		{
			_BackColorBrush = (uintptr_t)GetSysColorBrush(i);
			SetState(ControlStates::StateOwnCtlBrush, false);
		}
		else
		{
			_BackColorBrush = (uintptr_t)CreateSolidBrush(Color.ToCOLORREF());
			SetState(ControlStates::StateOwnCtlBrush, true);
		}

		return _BackColorBrush;
	}

	bool Control::RequiredScalingEnabled()
	{
		return this->_RequiredScalingEnabled;
	}

	void Control::SetRequiredScalingEnabled(bool Value)
	{
		this->_RequiredScalingEnabled = Value;
	}

	String Control::WindowText()
	{
		if (!GetState(ControlStates::StateCreated))
			return _Text;

		auto Length = GetWindowTextLengthA(this->_Handle);
		auto Result = String(Length);

		GetWindowTextA(this->_Handle, (char*)Result, Length + 1);

		return Result;
	}

	void Control::SetWindowText(const String& Value)
	{
		if (GetState(ControlStates::StateCreated))
			SetWindowTextA(this->_Handle, (char*)Value);
		else
			_Text = Value;
	}

	void Control::UpdateStyles()
	{
		if (GetState(ControlStates::StateCreated))
		{
			auto Cp = this->GetCreateParams();

			auto WinStyle = WindowStyle();
			auto ExStyle = WindowExStyle();

			if (GetState(ControlStates::StateVisible))
				Cp.Style |= WS_VISIBLE;

			if (WinStyle != Cp.Style)
				SetWindowStyle(Cp.Style);
			if (ExStyle != Cp.ExStyle)
				SetWindowExStyle(Cp.ExStyle);

			SetWindowPos(this->_Handle, NULL, 0, 0, 0, 0, SWP_DRAWFRAME | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);

			Invalidate(true);
		}

		OnStyleChanged();
	}

	void Control::SetAcceptDrops(bool Accept)
	{
		if (Accept != GetState(ControlStates::StateDropTarget) && GetState(ControlStates::StateCreated))
		{
			if (Accept)
			{
				this->_DropTarget = std::make_unique<DropTarget>(this);
				RegisterDragDrop(this->_Handle, (LPDROPTARGET)this->_DropTarget.get());
			}
			else
			{
				RevokeDragDrop(this->_Handle);
				this->_DropTarget.reset();
			}

			SetState(ControlStates::StateDropTarget, Accept);
		}
	}

	void Control::ResetMouseEventArgs()
	{
		if (GetState(ControlStates::StateTrackingMouseEvent))
		{
			SetState(ControlStates::StateTrackingMouseEvent, false);

			if (!GetState(ControlStates::StateTrackingMouseEvent))
			{
				SetState(ControlStates::StateTrackingMouseEvent, true);

				TRACKMOUSEEVENT mEvt{};
				mEvt.cbSize = sizeof(mEvt);
				mEvt.dwFlags = TME_LEAVE | TME_HOVER;
				mEvt.hwndTrack = this->_Handle;

				TrackMouseEvent(&mEvt);
			}
		}
	}

	void Control::DestroyHandle()
	{
		if (_Handle != nullptr)
		{
			DestroyWindow(_Handle);
			_Handle = nullptr;
		}
	}

	void Control::PerformLayout()
	{
		// This only applies if we house controls
		if (!GetStyle(ControlStyles::ContainerControl) || this->_Controls == nullptr)
			return;

		// We may need to not layout here...
		if (this->_LayoutSuspendCount > 0)
		{
			SetState(ControlStates::StateLayoutDeferred, true);
			return;
		}

		// By default, we will move all controls at once
		HDWP DeferHandle = nullptr;

		// Store the counts on the stack
		uint32_t ControlCount = this->_Controls->Count();
		auto& ControlList = *this->_Controls.get();

		RECT nRect{};
		::GetClientRect(this->_Handle, &nRect);

		// Calculate delta, must be signed integer
		int32_t DeltaX = ((nRect.right - nRect.left) - (this->_AnchorDeltas.InitialRect.right - this->_AnchorDeltas.InitialRect.left));
		int32_t DeltaY = ((nRect.bottom - nRect.top) - (this->_AnchorDeltas.InitialRect.bottom - this->_AnchorDeltas.InitialRect.top));

		for (uint32_t i = 0; i < ControlCount; i++)
		{
			auto Child = ControlList[i];
			auto ChildHwnd = Child->GetHandle();

			// Important: we must never defer null handles, OR the tooltip
			// otherwise the defer will never commit the changes...
			if (ChildHwnd == nullptr || Child->_RTTI == ControlTypes::ToolTip)
				continue;

			auto& ChildDeltas = Child->_AnchorDeltas;

			RECT RcNew(ChildDeltas.InitialRect);
			::OffsetRect(&RcNew, (int32_t)(DeltaX * ChildDeltas.XMoveFrac), (int32_t)(DeltaY * ChildDeltas.YMoveFrac));

			RcNew.right += (int32_t)(DeltaX * ChildDeltas.XSizeFrac);
			RcNew.bottom += (int32_t)(DeltaY * ChildDeltas.YSizeFrac);

			auto Width = (RcNew.right - RcNew.left);
			auto Height = (RcNew.bottom - RcNew.top);

			// Ensure we are within bounds, if necessary
			auto Minimum = Child->MinimumSize();
			auto Maximum = Child->MaximumSize();

			if (!Minimum.Empty())
			{
				Width = max(Minimum.Width, Width);
				Height = max(Minimum.Height, Height);
			}

			if (!Maximum.Empty())
			{
				Width = min(Maximum.Width, Width);
				Height = min(Maximum.Height, Height);
			}

			if (DeferHandle == nullptr)
				DeferHandle = BeginDeferWindowPos(ControlCount);

			auto Flags = SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER;

			// For controls with children that don't resize, we can speed this up by not invalidating the client rect
			if (ChildDeltas.XSizeFrac != 0.0f || ChildDeltas.YSizeFrac != 0.0f)
				Flags |= SWP_NOCOPYBITS;

			DeferWindowPos(DeferHandle, ChildHwnd, NULL, RcNew.left, RcNew.top, Width, Height, Flags);
		}

		// Layout was performed, we can mark completed
		SetState(ControlStates::StateLayoutDeferred | ControlStates::StateLayoutIsDirty, false);

		// End rendering
		if (DeferHandle != nullptr)
			EndDeferWindowPos(DeferHandle);
	}

	bool Control::ProcessKeyMessage(Message& Msg)
	{
		if (this->_Parent != nullptr && this->_Parent->ProcessKeyPreview(Msg))
			return true;

		return ProcessKeyEventArgs(Msg);
	}

	bool Control::ProcessKeyPreview(Message& Msg)
	{
		return this->_Parent == nullptr ? false : this->_Parent->ProcessKeyPreview(Msg);
	}

	bool Control::ProcessKeyEventArgs(Message& Msg)
	{
		// Define the events globally for reuse in events
		std::unique_ptr<KeyEventArgs> KeyEvent = nullptr;
		std::unique_ptr<KeyPressEventArgs> KeyPressEvent = nullptr;
		uintptr_t NewWParam = 0;

		if (Msg.Msg == WM_CHAR || Msg.Msg == WM_SYSCHAR)
		{
			KeyPressEvent = std::make_unique<KeyPressEventArgs>((char)Msg.WParam);
			OnKeyPress(KeyPressEvent);
			NewWParam = (uintptr_t)KeyPressEvent->KeyChar;
		}
		else
		{
			KeyEvent = std::make_unique<KeyEventArgs>((Keys)((int)Msg.WParam | (int)GetModifierKeys()));
			if (Msg.Msg == WM_KEYDOWN || Msg.Msg == WM_SYSKEYDOWN)
				OnKeyDown(KeyEvent);
			else
				OnKeyUp(KeyEvent);
		}

		if (KeyPressEvent != nullptr)
		{
			Msg.WParam = NewWParam;
			return KeyPressEvent->Handled();
		}
		else if (KeyEvent != nullptr)
		{
			if (KeyEvent->SuppressKeyPress())
			{
				RemovePendingMessages(WM_CHAR, WM_CHAR);
				RemovePendingMessages(WM_SYSCHAR, WM_SYSCHAR);
			}

			return KeyEvent->Handled();
		}

		// Security just incase we don't handle IME stuff...
		return false;
	}

	bool Control::GetVisibleCore()
	{
		if (!GetState(ControlStates::StateVisible))
			return false;
		else if (this->_Parent == nullptr)
			return true;
		else
			return this->_Parent->Visible();
	}

	void Control::SetVisibleCore(bool Value)
	{
		if (Visible() != Value)
		{
			bool fChange = false;

			if (GetState(ControlStates::StateTopLevel))
			{
				if (Value)
					ShowWindow(this->_Handle, SW_SHOW);
			}
			else if (this->_Handle != nullptr || Value && this->_Parent != nullptr && this->_Parent->GetState(ControlStates::StateCreated))
			{
				SetState(ControlStates::StateVisible, Value);
				fChange = true;

				SetWindowPos(this->_Handle, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | (Value ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));
			}

			if (Visible() != Value)
			{
				SetState(ControlStates::StateVisible, Value);
				fChange = true;
			}

			if (fChange)
			{
				OnVisibleChanged();
			}
		}
		else
		{
			if (!GetState(ControlStates::StateVisible) && !Value && this->_Handle != nullptr)
			{
				if (!IsWindowVisible(this->_Handle))
					return;
			}

			SetState(ControlStates::StateVisible, Value);

			if (this->_Handle != nullptr)
				SetWindowPos(this->_Handle, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | (Value ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));
		}
	}

	CreateParams Control::GetCreateParams()
	{
		CreateParams Result{};

		Result.X = this->_X;
		Result.Y = this->_Y;
		Result.Width = this->_Width;
		Result.Height = this->_Height;
		Result.Caption = this->_Text;

		Result.Style = WS_CLIPCHILDREN;
		if (GetStyle(ControlStyles::ContainerControl))
			Result.ExStyle |= WS_EX_CONTROLPARENT;
		Result.ClassStyle = CS_DBLCLKS;

		if (!GetState(ControlStates::StateTopLevel))
		{
			Result.Parent = (this->_Parent != nullptr) ? (uintptr_t)this->_Parent->GetHandle() : (uintptr_t)0;
			Result.Style |= WS_CHILD | WS_CLIPSIBLINGS;
		}

		if (GetState(ControlStates::StateTabstop)) 
			Result.Style |= WS_TABSTOP;
		if (GetState(ControlStates::StateVisible)) 
			Result.Style |= WS_VISIBLE;

		if (!Enabled())
			Result.Style |= WS_DISABLED;

		return Result;
	}

	LRESULT Control::InternalWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
	{
		// Catch WM_CREATE here so that we can properly set the class pointer
		if (Msg == WM_CREATE)
		{
			auto ControlClassPtr = (Control*)(((LPCREATESTRUCT)lParam)->lpCreateParams);
			SetWindowLongPtrA(hWnd, GWLP_USERDATA, (LONG_PTR)ControlClassPtr);
		}

		// Fetch the class data
		auto ClassPtr = GetWindowLongPtrA(hWnd, GWLP_USERDATA);
		auto ControlMessage = Message(hWnd, Msg, wParam, lParam);

		// Reflect into the proper WndProc
		if (ClassPtr == NULL)
			ControlMessage.Result = (uintptr_t)DefWindowProcA(hWnd, Msg, wParam, lParam);
		else
			((Control*)ClassPtr)->WndProc(ControlMessage);

		return (LRESULT)ControlMessage.Result;
	}

	void Control::AddControl(Control* Ctrl)
	{
		// Prepare to add the control, if we are created, create it, otherwise, wait
		this->_Controls->Add(Ctrl);

		// Check to initialize the control
		if (GetState(ControlStates::StateCreated))
		{
			Ctrl->CreateControl(this);
			SetAnchor(this->_Anchor);
		}
	}

	void Control::Dispose()
	{
		SetState(ControlStates::StateDisposing, true);

		if (GetState(ControlStates::StateOwnCtlBrush) && _BackColorBrush != (uintptr_t)0)
		{
			DeleteObject((HGDIOBJ)_BackColorBrush);
			_BackColorBrush = (uintptr_t)0;
		}

		this->DestroyHandle();

		OnHandleDestroyed();

		SetState(ControlStates::StateDisposed, true);
		SetState(ControlStates::StateDisposing, false);
	}

	uintptr_t Control::InitializeDCForWmCtlColor(HDC Dc, int32_t Message)
	{
		if (!GetStyle(ControlStyles::UserPaint))
		{
			SetTextColor(Dc, this->ForeColor().ToCOLORREF());
			SetBkColor(Dc, this->BackColor().ToCOLORREF());

			return BackColorBrush();
		}
		else
		{
			return (uintptr_t)GetStockObject(HOLLOW_BRUSH);
		}
	}

	bool Control::IsContainerControl()
	{
		return false;
	}

	HRGN Control::CreateCopyOfRgn(HRGN InRgn)
	{
		RECT Rect{};
		HRGN DestRgn;

		if (InRgn)
		{
			DestRgn = CreateRectRgnIndirect(&Rect);

			if (CombineRgn(DestRgn, InRgn, NULL, RGN_COPY) != ERROR)
				return DestRgn;

			DeleteObject(DestRgn);
		}

		return NULL;
	}

	void Control::WmMouseDown(Message& Msg, MouseButtons Button, uint32_t Clicks)
	{
		// Track that a key is pressed
		SetState(ControlStates::StateMousePressed, true);

		// Check for control message processing
		if (!GetStyle(ControlStyles::UserMouse))
			DefWndProc(Msg);
		else
		{
			// DefWndProc would normally set focus to the control, but
			// since we're skipping DefWndProc, we need to do it ourselves.
			if (Button == MouseButtons::Left && GetStyle(ControlStyles::Selectable))
				this->Focus();
		}

		// TODO: If not State2(MAINTAINSOWNCAPTUREMODE)
		SetCapture(true);

		if (this->Enabled())
			OnMouseDown(std::make_unique<MouseEventArgs>(Button, Clicks, LOWORD(Msg.LParam), HIWORD(Msg.LParam), 0));
	}

	void Control::WmMouseUp(Message& Msg, MouseButtons Button, uint32_t Clicks)
	{
		POINT Point;
		Point.x = LOWORD(Msg.LParam);
		Point.y = HIWORD(Msg.LParam);

		MapWindowPoints(this->_Handle, NULL, &Point, 1);	// Faster Control::PointToScreen();

		// Check for control message processing
		if (!GetStyle(ControlStyles::UserMouse))
		{
			DefWndProc(Msg);
		}
		else
		{
			// DefWndProc would normally trigger a context menu here
			// (for a right button click), but since we're skipping DefWndProc
			// we have to do it ourselves.
			if (Button == MouseButtons::Right)
				SendMessageA(this->_Handle, WM_CONTEXTMENU, (WPARAM)this->_Handle, MAKELPARAM(0, 0));
		}

		// Track whether or not it was a real click
		bool fClick = false;

		if (GetStyle(ControlStyles::StandardClick))
		{
			if (GetState(ControlStates::StateMousePressed) && WindowFromPoint(Point) == this->_Handle)
				fClick = true;
		}

		if (fClick)
		{
			if (!GetState(ControlStates::StateDoubleClickFired))
			{
				OnClick();
				OnMouseClick(std::make_unique<MouseEventArgs>(Button, Clicks, LOWORD(Msg.LParam), HIWORD(Msg.LParam), 0));
			}
			else
			{
				OnDoubleClick();
				OnMouseDoubleClick(std::make_unique<MouseEventArgs>(Button, 2, LOWORD(Msg.LParam), HIWORD(Msg.LParam), 0));
			}
		}

		// Call MouseUp after the click event
		OnMouseUp(std::make_unique<MouseEventArgs>(Button, Clicks, LOWORD(Msg.LParam), HIWORD(Msg.LParam), 0));

		// Reset the states
		SetState(ControlStates::StateDoubleClickFired |
			ControlStates::StateMousePressed |
			ControlStates::StateValidationCancelled, false);

		SetCapture(false);
	}

	void Control::WmMouseEnter(Message& Msg)
	{
		DefWndProc(Msg);
		OnMouseEnter();
	}

	void Control::WmMouseLeave(Message& Msg)
	{
		// We stop tracking once we leave the element
		SetState(ControlStates::StateTrackingMouseEvent, false);

		DefWndProc(Msg);
		OnMouseLeave();
	}

	void Control::WmMouseHover(Message& Msg)
	{
		DefWndProc(Msg);
		OnMouseHover();
	}

	void Control::WmClose(Message& Msg)
	{
		if (this->_Parent != nullptr)
		{
			HWND ParentHandle = this->_Handle;
			HWND LastParentHandle = ParentHandle;

			while (ParentHandle != nullptr)
			{
				LastParentHandle = ParentHandle;
				ParentHandle = GetParent(ParentHandle);

				auto Style = GetWindowLongA(LastParentHandle, GWL_STYLE);

				if ((Style & WS_CHILD) == 0)
					break;
			}

			if (LastParentHandle != nullptr)
				PostMessageA(LastParentHandle, WM_CLOSE, NULL, NULL);
		}

		DefWndProc(Msg);
	}

	void Control::WmEraseBkgnd(Message& Msg)
	{
		if (GetStyle(ControlStyles::UserPaint))
		{
			if (!GetStyle(ControlStyles::AllPaintingInWmPaint))
			{
				uintptr_t Dc = Msg.WParam;
				if (Dc == NULL)
				{
					Msg.Result = (uintptr_t)0;
					return;
				}

				RECT rcClient{};
				GetClientRect(this->_Handle, &rcClient);

				OnPaintBackground(std::make_unique<PaintEventArgs>((HDC)Dc, Drawing::Rectangle(rcClient.left, rcClient.top, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top)));
			}

			Msg.Result = (uintptr_t)1;
		}
		else
		{
			DefWndProc(Msg);
		}
	}

	void Control::WmPaint(Message& Msg)
	{
		auto IsDoubleBuffered = this->DoubleBuffered() || GetStyle(ControlStyles::AllPaintingInWmPaint) && GetStyle(ControlStyles::DoubleBuffer | ControlStyles::UserPaint);

		HWND hWnd = nullptr;
		HDC Dc = nullptr;
		PAINTSTRUCT Ps{};
		Drawing::Rectangle Clip;
		bool NeedsDisposeDc = false;

		if (Msg.WParam == NULL)
		{
			hWnd = this->_Handle;
			Dc = BeginPaint(hWnd, &Ps);

			if (!Dc)
				return;
			NeedsDisposeDc = true;

			Clip = Drawing::Rectangle(Ps.rcPaint.left, Ps.rcPaint.top, Ps.rcPaint.right - Ps.rcPaint.left, Ps.rcPaint.bottom - Ps.rcPaint.top);
		}
		else
		{
			Dc = (HDC)Msg.WParam;
			Clip = this->ClientRectangle();
		}

		if (!IsDoubleBuffered || (Clip.Width > 0 && Clip.Height > 0))
		{
			HPALETTE OldPal = nullptr;
			std::unique_ptr<Drawing::BufferedGraphics> BufferedGraphics;
			std::unique_ptr<PaintEventArgs> PaintEvent;
			Drawing::GraphicsState State = NULL;

			if (IsDoubleBuffered || Msg.WParam == NULL)
				OldPal = SetUpPalette(Dc, false, false);

			if (IsDoubleBuffered)
			{
				BufferedGraphics = std::make_unique<Drawing::BufferedGraphics>(Dc, this->ClientRectangle());
			}

			if (BufferedGraphics != nullptr)
			{
				BufferedGraphics->Graphics->SetClip(Clip);
				PaintEvent = std::make_unique<PaintEventArgs>(BufferedGraphics->Graphics.get(), Clip);
				State = PaintEvent->Graphics->Save();
			}
			else
			{
				PaintEvent = std::make_unique<PaintEventArgs>(Dc, Clip);
			}

			if ((Msg.WParam == NULL) && GetStyle(ControlStyles::AllPaintingInWmPaint) || IsDoubleBuffered)
				OnPaintBackground(PaintEvent);

			if (State != NULL)
				PaintEvent->Graphics->Restore(State);
			else
				PaintEvent->ResetGraphics();

			OnPaint(PaintEvent);

			if (BufferedGraphics != nullptr)
			{
				BufferedGraphics->Render();
				PaintEvent->Graphics.release();
			}

			if (OldPal != nullptr)
				SelectPalette(Dc, OldPal, FALSE);
		}

		if (NeedsDisposeDc)
			EndPaint(hWnd, &Ps);
	}

	void Control::WmCreate(Message& Msg)
	{
		DefWndProc(Msg);
	}

	void Control::WmShowWindow(Message& Msg)
	{
		DefWndProc(Msg);

		if (!GetState(ControlStates::StateRecreate))
		{
			bool isVisible = (Msg.WParam != NULL);
			bool oVisibleProperty = Visible();

			if (isVisible)
			{
				// This doesn't match .NET because we don't create the control here, we just ensure the bit is set
				SetState(ControlStates::StateVisible, true);
			}
			else
			{
				bool pVisible = GetState(ControlStates::StateTopLevel);
				if (this->_Parent != nullptr)
					pVisible = this->_Parent->Visible();

				if (pVisible)
					SetState(ControlStates::StateVisible, false);
			}

			if (!GetState(ControlStates::StateParentRecreating) && (oVisibleProperty != isVisible))
				OnVisibleChanged();
		}
	}

	void Control::WmMove(Message& Msg)
	{
		DefWndProc(Msg);
		UpdateBounds();
	}

	void Control::WmParentNotify(Message& Msg)
	{
		auto Mid = LOWORD(Msg.WParam);
		HWND hWnd = nullptr;

		switch (Mid)
		{
		case WM_CREATE:
			hWnd = (HWND)Msg.LParam;
			break;
		case WM_DESTROY:
			break;
		default:
			hWnd = GetDlgItem(this->_Handle, HIWORD(Msg.WParam));
			break;
		}

		if (hWnd == nullptr || !ReflectMessageInternal(hWnd, Msg))
			DefWndProc(Msg);
	}

	void Control::WmCommand(Message& Msg)
	{
		if (Msg.LParam == NULL)
		{
			// TODO: Command.DispatchID();
		}
		else
		{
			if (ReflectMessageInternal((HWND)Msg.LParam, Msg))
				return;
		}

		DefWndProc(Msg);
	}

	void Control::WmQueryNewPalette(Message& Msg)
	{
		auto Dc = GetDC(this->_Handle);

		SetUpPalette(Dc, true, true);

		ReleaseDC(this->_Handle, Dc);

		Invalidate(true);
		Msg.Result = (uintptr_t)1;

		DefWndProc(Msg);
	}

	void Control::WmNotify(Message& Msg)
	{
		auto nMDR = (NMHDR*)Msg.LParam;

		if (!ReflectMessageInternal(nMDR->hwndFrom, Msg))
		{
			if (nMDR->code == TTN_SHOW)
			{
				SendMessageA(nMDR->hwndFrom, WM_REFLECT + Msg.Msg, (WPARAM)Msg.WParam, (LPARAM)Msg.LParam);
				return;
			}

			if (nMDR->code == TTN_POP)
				SendMessageA(nMDR->hwndFrom, WM_REFLECT + Msg.Msg, (WPARAM)Msg.WParam, (LPARAM)Msg.LParam);

			DefWndProc(Msg);
		}
	}

	void Control::WmNotifyFormat(Message& Msg)
	{
		if (!ReflectMessageInternal((HWND)Msg.WParam, Msg))
			DefWndProc(Msg);
	}

	void Control::WmCaptureChanged(Message& Msg)
	{
		OnMouseCaptureChanged();
		DefWndProc(Msg);
	}

	void Control::WmCtlColorControl(Message& Msg)
	{
		auto ControlPtr = GetWindowLongPtrA((HWND)Msg.LParam, GWLP_USERDATA);
		if (ControlPtr != NULL)
		{
			Msg.Result = ((Control*)ControlPtr)->InitializeDCForWmCtlColor((HDC)Msg.WParam, Msg.Msg);
			if (Msg.Result != NULL)
				return;
		}

		DefWndProc(Msg);
	}

	void Control::WmKillFocus(Message& Msg)
	{
		DefWndProc(Msg);
		OnLostFocus();
	}

	void Control::WmSetFocus(Message& Msg)
	{
		ContainerControl* C = (ContainerControl*)GetContainerControl();
		
		if (C != nullptr)
		{
			if (!C->ActivateControl(this))
				return;
		}

		DefWndProc(Msg);
		OnGotFocus();
	}

	void Control::WmMouseMove(Message& Msg)
	{
		if (!GetState(ControlStates::StateTrackingMouseEvent))
		{
			SetState(ControlStates::StateTrackingMouseEvent, true);

			TRACKMOUSEEVENT mEvent{};
			mEvent.cbSize = sizeof(mEvent);
			mEvent.dwFlags = TME_LEAVE | TME_HOVER;
			mEvent.hwndTrack = this->_Handle;

			TrackMouseEvent(&mEvent);

			if (!GetState(ControlStates::StateMouseEnterPending))
				SendMessageA(this->_Handle, WM_MOUSEENTER, 0, 0);
			else
				SetState(ControlStates::StateMouseEnterPending, false);
		}

		if (!GetStyle(ControlStyles::UserMouse))
			DefWndProc(Msg);
		else
			OnMouseMove(std::make_unique<MouseEventArgs>(Control::GetMouseButtons(), 0, (int16_t)LOWORD(Msg.LParam), (int16_t)HIWORD(Msg.LParam), 0));
	}

	void Control::WmSetCursor(Message& Msg)
	{
		if ((HWND)Msg.WParam == this->_Handle && LOWORD(Msg.LParam) == HTCLIENT)
		{
			// TODO: Cursor.CursorInternal = Cursor;
			// Remove defwndproc...
			DefWndProc(Msg);
		}
		else
		{
			DefWndProc(Msg);
		}
	}

	void Control::WmMouseWheel(Message& Msg)
	{
		auto mPoint = this->PointToClient({ LOWORD(Msg.LParam), HIWORD(Msg.LParam) });

		auto eArgs = std::make_unique<HandledMouseEventArgs>(MouseButtons::None, 0, mPoint.X, mPoint.Y, (int16_t)HIWORD(Msg.WParam));

		OnMouseWheel(eArgs);
		if (!eArgs->Handled)
		{
			DefWndProc(Msg);
		}
	}

	void Control::WmKeyChar(Message& Msg)
	{
		if (ProcessKeyMessage(Msg))
			return;

		DefWndProc(Msg);
	}

	void Control::WmWindowPosChanged(Message& Msg)
	{
		DefWndProc(Msg);
		UpdateBounds();

		if (this->_Parent != nullptr && GetParent(this->_Handle) == this->_Parent->GetHandle() && !GetState(ControlStates::StateNoZOrder))
		{
			auto wPos = (WINDOWPOS*)Msg.LParam;
			if ((wPos->flags & SWP_NOZORDER) == 0)
			{
				this->_Parent->UpdateChildControlIndex(this);
			}
		}
	}

	void Control::WmInvokeOnUIThread(Message& Msg)
	{
		if (Msg.LParam == NULL)
			return;

		// Invoke the function here
		((void(*)(void))Msg.LParam)();
	}

	void Control::RemovePendingMessages(uint32_t MsgMin, uint32_t MsgMax)
	{
		MSG Msg{};
		while (PeekMessageA(&Msg, this->_Handle, MsgMin, MsgMax, PM_REMOVE));
	}

	void Control::UpdateChildZOrder(Control* Ctrl)
	{
		if (!GetState(ControlStates::StateCreated) || Ctrl->_Parent != this)
			return;

		HWND PrevHandle = (HWND)HWND_TOP;
		auto& ControlList = *this->_Controls.get();

		for (int32_t i = this->_Controls->IndexOf(Ctrl); --i >= 0;)
		{
			Control* C = ControlList[i];
			if (C->GetState(ControlStates::StateCreated) && C->_Parent == this)
			{
				PrevHandle = C->_Handle;
				break;
			}
		}

		if (GetWindow(Ctrl->_Handle, GW_HWNDPREV) != PrevHandle)
		{
			SetState(ControlStates::StateNoZOrder, true);
			SetWindowPos(Ctrl->_Handle, PrevHandle, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			SetState(ControlStates::StateNoZOrder, false);
		}
	}

	void Control::UpdateChildControlIndex(Control* Ctrl)
	{
		// TODO: If we implement a tab control, we MUST prevent this from continuing here...
		// See: https://referencesource.microsoft.com/#System.Windows.Forms/winforms/Managed/System/WinForms/Control.cs,12635

		int32_t NewIndex = 0;
		int32_t CurIndex = this->_Controls->IndexOf(Ctrl);
		auto Hwnd = Ctrl->_Handle;

		while ((Hwnd = GetWindow(Hwnd, GW_HWNDPREV)) != NULL)
		{
			Control* C = Control::FromHandle(Hwnd);
			if (C != nullptr)
			{
				NewIndex = this->_Controls->IndexOf(C) + 1;
				break;
			}
		}

		if (NewIndex > CurIndex)
			NewIndex--;

		if (NewIndex != CurIndex)
			this->_Controls->SetChildIndex(Ctrl, NewIndex);
	}

	void Control::SelectNextIfFocused()
	{
		if (ContainsFocus() && this->_Parent != nullptr)
		{
			auto C = (ContainerControl*)this->_Parent->GetContainerControl();

			if (C != nullptr)
				C->SelectNextControl(this, true, true, true, true);
		}
	}

	Control* Control::GetNextSelectableControl(Control* Ctrl, bool Forward, bool TabStopOnly, bool Nested, bool Wrap)
	{
		if (!Contains(Ctrl) || !Nested && Ctrl->_Parent != this)
			Ctrl = nullptr;

		bool AlreadyWrapped = false;
		Control* Start = Ctrl;

		do
		{
			Ctrl = GetNextControl(Ctrl, Forward);
			if (Ctrl == nullptr)
			{
				if (!Wrap)
					break;
				if (AlreadyWrapped)
					return nullptr;

				AlreadyWrapped = true;
			}
			else
			{
				if (Ctrl->CanSelect() && (Nested || Ctrl->_Parent == this))	// TODO: TabStopOnly / TabStop...
					return Ctrl;
			}
		} while (Ctrl != Start);

		return nullptr;
	}

	Control* Control::GetFirstChildcontrolInTabOrder(bool Forward)
	{
		if (this->_Controls == nullptr)
			return nullptr;

		Control* Found = nullptr;
		auto& ControlList = *this->_Controls.get();

		if (Forward)
		{
			for (uint32_t i = 0; i < this->_Controls->Count(); i++)
			{
				if (Found == nullptr || Found->_TabIndex > ControlList[i]->_TabIndex)
					Found = ControlList[i];
			}
		}
		else
		{
			for (int32_t i = (int32_t)this->_Controls->Count() - 1; i >= 0; i--)
			{
				if (Found == nullptr || Found->_TabIndex < ControlList[i]->_TabIndex)
					Found = ControlList[i];
			}
		}

		return Found;
	}

	Control* Control::FromHandle(HWND hWnd)
	{
		if (hWnd == NULL)
			return nullptr;

		return (Control*)GetWindowLongPtrA(hWnd, GWLP_USERDATA);
	}

	Control* Control::FromChildHandle(HWND hWnd)
	{
		while (hWnd != NULL)
		{
			auto Ctrl = Control::FromHandle(hWnd);
			if (Ctrl != nullptr)
				return Ctrl;

			hWnd = GetAncestor(hWnd, GA_PARENT);
		}

		return nullptr;
	}

	bool Control::ReflectMessageInternal(HWND hWnd, Message& Msg)
	{
		auto Control = GetWindowLongPtrA(hWnd, GWLP_USERDATA);
		if (Control == NULL)
			return false;

		Msg.Result = SendMessageA(hWnd, WM_REFLECT + Msg.Msg, Msg.WParam, Msg.LParam);

		return true;
	}

	bool Control::IsFocusManagingContainerControl(Control* Ctrl)
	{
		return ((Ctrl->GetStyle(ControlStyles::ContainerControl) && Ctrl->IsContainerControl()));
	}

	String Control::RegisterWndClass(const char* ClassName, DWORD ClassStyle, bool& Subclass)
	{
		// Check for a built-in class name...
		WNDCLASSEXA ExInfo{};
		ExInfo.cbSize = sizeof(WNDCLASSEXA);

		// Default windows classes
		const char* const DefWndClasses[] =
		{ 
			"Button",				// A standard button, checkbox, radio, etc
			"ComboBox",				// A combobox control
			"Edit",					// A textbox / multiline edit box
			"ListBox",				// A listbox control
			"MDIClient",			// MDIClient child window
			"ScrollBar",			// A scrollbar
			"Static",				// Any static text
			"msctls_progress32",	// A progress bar
			"tooltips_class32",		// Creates tooltip controls
			"msctls_trackbar32",	// Creates a range slider control
			"msctls_updown32",		// Creates an integral range edit box
			"SysListView32",		// Creates listview controls, extended listbox
			"SysTabControl32",		// Creates a tabcontrol
			"SysTreeView32",		// Creates a treeview control
		};

		for (auto& Class : DefWndClasses)
		{
			if (_strnicmp(ClassName, Class, strlen(ClassName)) == 0)
			{
				Subclass = true;
				return ClassName;
			}
		}

		// Check if we already registered this class...
		if (GetClassInfoExA(GetModuleHandle(NULL), ClassName, &ExInfo))
		{
			// This is an existing class...
			if (ExInfo.style == ClassStyle && ExInfo.lpfnWndProc == (WNDPROC)&Control::InternalWndProc)
				return ClassName;

			// We need to make a modified class, using the ClassName + Style
			auto nClassName = String(ClassName) + String::Format(".%x", ClassStyle);

			ExInfo.style = ClassStyle;
			ExInfo.lpszClassName = (const char*)nClassName;
			ExInfo.hCursor = LoadCursor(NULL, IDC_ARROW);

			Subclass = true;

			RegisterClassExA(&ExInfo);

			return nClassName;
		}
		else
		{
			// This is a non-existing class, register everything
			ExInfo.style = ClassStyle;
			ExInfo.lpszClassName = ClassName;
			ExInfo.lpfnWndProc = (WNDPROC)&Control::InternalWndProc;
			ExInfo.hInstance = GetModuleHandle(NULL);
			ExInfo.hbrBackground = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
			ExInfo.hCursor = LoadCursor(NULL, IDC_ARROW);

			Subclass = false;
			
			RegisterClassExA(&ExInfo);

			return String(ClassName);
		}
	}
}