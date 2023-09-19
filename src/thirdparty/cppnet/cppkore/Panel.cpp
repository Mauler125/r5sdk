#include "stdafx.h"
#include "Panel.h"

namespace Forms
{
	Panel::Panel()
		: Control()
	{
		SetStyle(ControlStyles::Selectable | ControlStyles::AllPaintingInWmPaint, false);
		SetStyle(ControlStyles::SupportsTransparentBackColor | ControlStyles::ContainerControl, true);

		// Setup the container
		this->_Controls = std::make_unique<ControlCollection>();

		// We are a panel control
		this->_RTTI = ControlTypes::Panel;
	}

	BorderStyle Panel::GetBorderStyle()
	{
		return this->_BorderStyle;
	}

	void Panel::SetBorderStyle(BorderStyle Value)
	{
		this->_BorderStyle = Value;
		UpdateStyles();
	}

	void Panel::AddControl(Control* Ctrl)
	{
		Control::AddControl(Ctrl);
	}

	CreateParams Panel::GetCreateParams()
	{
		auto Cp = Control::GetCreateParams();

		Cp.ClassName = "PANEL";
		Cp.ExStyle |= WS_EX_CONTROLPARENT;

		Cp.ExStyle &= (~WS_EX_CLIENTEDGE);
		Cp.Style &= (~WS_BORDER);

		switch (_BorderStyle)
		{
		case BorderStyle::Fixed3D:
			Cp.ExStyle |= WS_EX_CLIENTEDGE;
			break;
		case BorderStyle::FixedSingle:
			Cp.Style |= WS_BORDER;
			break;
		}

		return Cp;
	}
}
