#include "stdafx.h"
#include "Message.h"
#include "BufferedGraphics.h"
#include "UIXTheme.h"
#include "UIXListViewHeader.h"

namespace UIX
{
	UIXListViewHeader::UIXListViewHeader(HWND Handle, Forms::ListView* Parent)
		: _Handle(Handle), _Parent(Parent)
	{
		SetWindowSubclass(Handle, &UIXListViewHeader::InternalWndProc, NULL, (DWORD_PTR)this);
	}

	void UIXListViewHeader::WndProc(Forms::Message& Msg)
	{
		// Used to allow for custom theming of the background of this control
		switch (Msg.Msg)
		{
		case WM_ERASEBKGND:
			Msg.Result = (intptr_t)1;
			break;
		case WM_PAINT:
		{
			// Paint the original items
			DefWndProc(Msg);

			RECT rcClient{};
			RECT lastRc{};

			// Get items
			auto ItemCount = SendMessageA(this->_Handle, HDM_GETITEMCOUNT, NULL, NULL);

			// Calculate bounds of last item
			GetClientRect(this->_Handle, &rcClient);
			SendMessageA(this->_Handle, HDM_GETITEMRECT, (WPARAM)ItemCount - 1, (LPARAM)&lastRc);

			// We only need to render the list header if we are outside of the bounds
			if (lastRc.right < rcClient.right)
			{
				auto Dc = GetWindowDC(this->_Handle);
				{
					UIXTheme::GetRenderer()->RenderControlListHeader(std::make_unique<Drawing::BufferedGraphics>((HDC)Dc, Drawing::Rectangle(lastRc.right, 0, rcClient.right - lastRc.right, lastRc.bottom)));
				}
				ReleaseDC(this->_Handle, Dc);
			}
		}
		break;
		default:
			DefWndProc(Msg);
			break;
		}
	}

	void UIXListViewHeader::DefWndProc(Forms::Message& Msg)
	{
		// Proxy to subclassed func
		Msg.Result = DefSubclassProc((HWND)Msg.HWnd, Msg.Msg, Msg.WParam, Msg.LParam);
	}

	LRESULT UIXListViewHeader::InternalWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
	{
		// Fetch the class data
		auto ControlMessage = Forms::Message(hWnd, Msg, wParam, lParam);
		((UIXListViewHeader*)dwRefData)->WndProc(ControlMessage);

		// Return result
		return (LRESULT)ControlMessage.Result;
	}
}
