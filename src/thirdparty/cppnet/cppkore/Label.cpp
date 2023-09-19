#include "stdafx.h"
#include "Label.h"

namespace Forms
{
	Label::Label()
		: Control(), _OwnerDraw(false), _FlatStyle(FlatStyle::Standard), _TextAlign(Drawing::ContentAlignment::TopLeft), _BorderStyle(BorderStyle::None)
	{
		SetStyle(ControlStyles::UserPaint |
			ControlStyles::SupportsTransparentBackColor |
			ControlStyles::OptimizedDoubleBuffer, this->_OwnerDraw);

		SetStyle(ControlStyles::Selectable |
			ControlStyles::FixedHeight, false);

		SetStyle(ControlStyles::ResizeRedraw, true);

		// We are a label control
		this->_RTTI = ControlTypes::Label;
	}

	bool Label::OwnerDraw()
	{
		return this->_OwnerDraw;
	}

	void Label::SetOwnerDraw(bool Value)
	{
		this->_OwnerDraw = Value;

		SetStyle(ControlStyles::UserPaint | ControlStyles::SupportsTransparentBackColor | ControlStyles::OptimizedDoubleBuffer, Value);

		UpdateStyles();
		Invalidate();
	}

	Drawing::ContentAlignment Label::TextAlign()
	{
		return this->_TextAlign;
	}

	void Label::SetTextAlign(Drawing::ContentAlignment Value)
	{
		this->_TextAlign = Value;

		if (this->_OwnerDraw)
			Invalidate();
		else
			UpdateStyles();
	}

	FlatStyle Label::GetFlatStyle()
	{
		return this->_FlatStyle;
	}

	void Label::SetFlatStyle(FlatStyle Value)
	{
		this->_FlatStyle = Value;

		Invalidate();

		// Force update styles...
		SetStyle(ControlStyles::UserPaint | ControlStyles::SupportsTransparentBackColor | ControlStyles::OptimizedDoubleBuffer, this->_OwnerDraw);
		UpdateStyles();
	}

	BorderStyle Label::GetBorderStyle()
	{
		return this->_BorderStyle;
	}

	void Label::SetBorderStyle(BorderStyle Value)
	{
		this->_BorderStyle = Value;

		Invalidate();

		// Force update styles...
		SetStyle(ControlStyles::UserPaint | ControlStyles::SupportsTransparentBackColor | ControlStyles::OptimizedDoubleBuffer, this->_OwnerDraw);
		UpdateStyles();
	}

	void Label::WndProc(Message& Msg)
	{
		switch (Msg.Msg)
		{
		case WM_NCHITTEST:
		{
			auto RcScreen = this->RectangleToScreen(Drawing::Rectangle(0, 0, this->_Width, this->_Height));

			// A label normally returns HT_TRANSPARENT, we can modify this by calculating the
			// hits ourself here
			Msg.Result = (RcScreen.Contains(LOWORD(Msg.LParam), HIWORD(Msg.LParam)) ? HTCLIENT : HTNOWHERE);
			break;
		}
		case WM_SETTEXT:
		{
			Control::WndProc(Msg);

			// A lable that is SS_OWNERDRAW does not render properly during WM_SETTEXT
			// we must trigger this...
			if (this->OwnerDraw())
				this->Invalidate(true);

			break;
		}
		default:
			Control::WndProc(Msg);
			break;
		}
	}

	CreateParams Label::GetCreateParams()
	{
		auto Cp = Control::GetCreateParams();
		Cp.ClassName = "STATIC";

		if (this->_OwnerDraw)
		{
			Cp.Style |= SS_OWNERDRAW;
			Cp.ExStyle &= ~WS_EX_RIGHT;
		}

		if (!this->_OwnerDraw)
		{
			switch (this->_TextAlign)
			{
			case Drawing::ContentAlignment::TopLeft:
			case Drawing::ContentAlignment::MiddleLeft:
			case Drawing::ContentAlignment::BottomLeft:
				Cp.Style |= SS_LEFT;
				break;

			case Drawing::ContentAlignment::TopRight:
			case Drawing::ContentAlignment::MiddleRight:
			case Drawing::ContentAlignment::BottomRight:
				Cp.Style |= SS_RIGHT;
				break;

			case Drawing::ContentAlignment::TopCenter:
			case Drawing::ContentAlignment::MiddleCenter:
			case Drawing::ContentAlignment::BottomCenter:
				Cp.Style |= SS_CENTER;
				break;
			}

			// Trick to vertically center built-in label text
			if (((int)this->_TextAlign & (int)Drawing::AnyMiddleAlign) != 0)
				Cp.Style |= SS_CENTERIMAGE;
		}
		else
			Cp.Style |= SS_LEFT;

		switch (this->_BorderStyle)
		{
		case BorderStyle::FixedSingle:
			Cp.Style |= WS_BORDER;
			break;
		case BorderStyle::Fixed3D:
			Cp.Style |= SS_SUNKEN;
			break;
		}

		Cp.Style |= SS_NOPREFIX;

		return Cp;
	}
}
