#include "stdafx.h"
#include "GroupBox.h"

namespace Forms
{
	GroupBox::GroupBox()
		: Control(), _OwnerDraw(false), _FlatStyle(FlatStyle::Standard)
	{
		SetStyle(ControlStyles::ContainerControl, true);

		SetStyle(ControlStyles::SupportsTransparentBackColor |
			ControlStyles::UserPaint |
			ControlStyles::ResizeRedraw, this->_OwnerDraw);

		SetStyle(ControlStyles::Selectable, false);

		// Setup the container
		this->_Controls = std::make_unique<ControlCollection>();

		// We are a group box control
		this->_RTTI = ControlTypes::GroupBox;
	}

	bool GroupBox::OwnerDraw()
	{
		return this->_OwnerDraw;
	}

	void GroupBox::SetOwnerDraw(bool Value)
	{
		this->_OwnerDraw = Value;

		SetStyle(ControlStyles::SupportsTransparentBackColor | ControlStyles::UserPaint | ControlStyles::ResizeRedraw, this->_OwnerDraw);

		UpdateStyles();
		Invalidate();
	}

	FlatStyle GroupBox::GetFlatStyle()
	{
		return this->_FlatStyle;
	}

	void GroupBox::SetFlatStyle(FlatStyle Value)
	{
		this->_FlatStyle = Value;

		Invalidate();

		// Force update styles...
		SetStyle(ControlStyles::SupportsTransparentBackColor | ControlStyles::UserPaint | ControlStyles::ResizeRedraw, this->_OwnerDraw);
		UpdateStyles();
	}

	void GroupBox::AddControl(Control* Ctrl)
	{
		Control::AddControl(Ctrl);
	}

	void GroupBox::WndProc(Message& Msg)
	{
		if (this->_OwnerDraw)
		{
			Control::WndProc(Msg);
			return;
		}

		switch (Msg.Msg)
		{
		case WM_ERASEBKGND:
		case WM_PRINTCLIENT:
			WmEraseBkgnd(Msg);
			break;
		case WM_GETOBJECT:
			Control::WndProc(Msg);

			// This allows MSAA to traverse the children of this control when we use
			// the button class
			if ((int)Msg.LParam == OBJID_QUERYCLASSNAMEIDX)
				Msg.Result = (uintptr_t)0;
			break;
		default:
			Control::WndProc(Msg);
			break;
		}
	}

	CreateParams GroupBox::GetCreateParams()
	{
		auto Cp = Control::GetCreateParams();

		if (!this->_OwnerDraw)
		{
			Cp.ClassName = "BUTTON";
			Cp.Style |= BS_GROUPBOX;
		}
		else
		{
			Cp.ClassName = "";
			Cp.Style &= ~BS_GROUPBOX;
		}

		return Cp;
	}

	void GroupBox::WmEraseBkgnd(Message& Msg)
	{
		RECT Rc{};
		GetClientRect(this->_Handle, &Rc);

		auto Gfx = Drawing::Graphics::FromHDC((HDC)Msg.WParam);
		auto Brush = Drawing::SolidBrush(this->BackColor());

		if (Gfx)
		{
			Gfx->FillRectangle(&Brush, Drawing::Rectangle(Rc.left, Rc.top, Rc.right - Rc.left, Rc.bottom - Rc.top));
			delete Gfx;
		}

		// Handled
		Msg.Result = (uintptr_t)1;
	}
}
