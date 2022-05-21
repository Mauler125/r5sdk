#include "stdafx.h"
#include "MessageBox.h"

namespace Forms
{
	DialogResult MessageBox::Win32ToDialogResult(uint32_t Value)
	{
		switch (Value)
		{
		case IDOK:
			return DialogResult::OK;
		case IDCANCEL:
			return DialogResult::Cancel;
		case IDABORT:
			return DialogResult::Abort;
		case IDRETRY:
			return DialogResult::Retry;
		case IDIGNORE:
			return DialogResult::Ignore;
		case IDYES:
			return DialogResult::Yes;
		case IDNO:
			return DialogResult::No;
		default:
			return DialogResult::No;
		}
	}

	static Drawing::Color ForegroundColor = Drawing::Color::Black;
	static Drawing::Color BackgroundColor = Drawing::Color::White;
	static Drawing::Color BottomColor = Drawing::Color(240, 240, 240);

	struct Win32MessageBoxThreadInfo
	{
		HHOOK hHook;				// Hook handle
		HWND  hWnd;					// Message box handle
		WNDPROC lpMsgBoxProc;		// Original window proc
		COLORREF crText;			// Foreground color
		COLORREF crBack;			// Background color
		COLORREF crBottom;			// Background bottom color
		HBRUSH hBrush;				// Background brush
		HBRUSH hBrushBottom;		// Background bottom brush
	} static thread_local Win32PerThreadInfo;

	LRESULT CALLBACK Win32MessageBoxProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_INITDIALOG:
		{
			Win32PerThreadInfo.hBrush = ::CreateSolidBrush(Win32PerThreadInfo.crBack);
			Win32PerThreadInfo.hBrushBottom = ::CreateSolidBrush(Win32PerThreadInfo.crBottom);
		}
		break;
		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
		{
			HDC hDC = (HDC)wParam;

			::SetBkMode(hDC, TRANSPARENT);
			::SetTextColor(hDC, Win32PerThreadInfo.crText);

			return (LRESULT)Win32PerThreadInfo.hBrush;
		}
		case WM_CTLCOLORBTN:
		{
			return (LRESULT)Win32PerThreadInfo.hBrushBottom;
		}
		case WM_PAINT:
		{
			auto Result = CallWindowProc(Win32PerThreadInfo.lpMsgBoxProc, hWnd, uMsg, wParam, lParam);;

			RECT Rc{};
			GetClientRect(hWnd, &Rc);

			auto DC = GetDC(hWnd);

			FillRect(DC, &Rc, Win32PerThreadInfo.hBrush);

			Rc.top = Rc.bottom - 42;

			FillRect(DC, &Rc, Win32PerThreadInfo.hBrushBottom);

			return Result;
		}
		case WM_COMMAND:
		{
			::DeleteObject(Win32PerThreadInfo.hBrush);
			::DeleteObject(Win32PerThreadInfo.hBrushBottom);
		}
		break;
		}

		return CallWindowProc(Win32PerThreadInfo.lpMsgBoxProc, hWnd, uMsg, wParam, lParam);
	}

	LRESULT CALLBACK Win32CallbackProc(int nCode, WPARAM wParam, LPARAM lParam)
	{
		if (nCode < 0)
			return ::CallNextHookEx(Win32PerThreadInfo.hHook, nCode, wParam, lParam);

		switch (nCode)
		{
		case HCBT_CREATEWND:
		{
			// Ensure that we have the right window to track
			LPCBT_CREATEWND lpCbtCreate = (LPCBT_CREATEWND)lParam;
			if (WC_DIALOG == lpCbtCreate->lpcs->lpszClass)
			{
				Win32PerThreadInfo.hWnd = (HWND)wParam;
			}
			else
			{
				if ((NULL == Win32PerThreadInfo.lpMsgBoxProc) && (NULL != Win32PerThreadInfo.hWnd))
					Win32PerThreadInfo.lpMsgBoxProc = (WNDPROC)::SetWindowLongPtr(Win32PerThreadInfo.hWnd, GWLP_WNDPROC, (LONG_PTR)Win32MessageBoxProc);
			}
		}
		break;
		case HCBT_DESTROYWND:
		{
			// Ensure that our tracked window is being destroyed
			if (Win32PerThreadInfo.hWnd == (HWND)wParam)
				::SetWindowLongPtr(Win32PerThreadInfo.hWnd, GWLP_WNDPROC, (LONG_PTR)Win32PerThreadInfo.lpMsgBoxProc);
		}
		}

		return 0;
	}

	DialogResult MessageBox::ShowCore(const string& Text, const string& Caption, MessageBoxButtons Buttons, MessageBoxIcon Icon, MessageBoxDefaultButton DefaultButton, MessageBoxOptions Options, bool ShowHelp, Control* Owner)
	{
		uint32_t Style = (ShowHelp) ? HELP_BUTTON : 0;
		Style |= (int)Buttons | (int)Icon | (int)DefaultButton | (int)Options;

		HWND Handle = nullptr;

		if (ShowHelp || (((int)Options & ((int)MessageBoxOptions::ServiceNotification | (int)MessageBoxOptions::DefaultDesktopOnly)) == 0))
		{
			if (Owner == nullptr)
				Handle = GetActiveWindow();
			else
				Handle = Owner->GetHandle();
		}

		// Reset hooking data for this thread
		Win32PerThreadInfo.hHook = NULL;
		Win32PerThreadInfo.hWnd = NULL;
		Win32PerThreadInfo.lpMsgBoxProc = NULL;

		// Load the initial color values
		Win32PerThreadInfo.crText = RGB(ForegroundColor.GetR(), ForegroundColor.GetG(), ForegroundColor.GetB());
		Win32PerThreadInfo.crBack = RGB(BackgroundColor.GetR(), BackgroundColor.GetG(), BackgroundColor.GetB());
		Win32PerThreadInfo.crBottom = RGB(BottomColor.GetR(), BottomColor.GetG(), BottomColor.GetB());

		// Install the hook
		Win32PerThreadInfo.hHook = ::SetWindowsHookExA(WH_CBT, Win32CallbackProc, GetModuleHandle(NULL), GetCurrentThreadId());

		auto Result = MessageBoxA(Handle, (const char*)Text, (const char*)Caption, Style);

		// Remove the hook
		::UnhookWindowsHookEx(Win32PerThreadInfo.hHook);

		// Convert the result
		return Win32ToDialogResult(Result);
	}

	DialogResult MessageBox::Show(const string& Text)
	{
		return ShowCore(Text, "", MessageBoxButtons::OK, MessageBoxIcon::None, MessageBoxDefaultButton::Button1, (MessageBoxOptions)0, false, nullptr);
	}

	DialogResult MessageBox::Show(const string& Text, const string& Caption)
	{
		return ShowCore(Text, Caption, MessageBoxButtons::OK, MessageBoxIcon::None, MessageBoxDefaultButton::Button1, (MessageBoxOptions)0, false, nullptr);
	}

	DialogResult MessageBox::Show(const string& Text, const string& Caption, MessageBoxButtons Buttons)
	{
		return ShowCore(Text, Caption, Buttons, MessageBoxIcon::None, MessageBoxDefaultButton::Button1, (MessageBoxOptions)0, false, nullptr);
	}

	DialogResult MessageBox::Show(const string& Text, const string& Caption, MessageBoxButtons Buttons, MessageBoxIcon Icon)
	{
		return ShowCore(Text, Caption, Buttons, Icon, MessageBoxDefaultButton::Button1, (MessageBoxOptions)0, false, nullptr);
	}

	DialogResult MessageBox::Show(Control* Owner, const string& Text, const string& Caption, MessageBoxButtons Buttons, MessageBoxIcon Icon)
	{
		return ShowCore(Text, Caption, Buttons, Icon, MessageBoxDefaultButton::Button1, (MessageBoxOptions)0, false, Owner);
	}

	DialogResult MessageBox::Show(Control* Owner, const string& Text, const string& Caption, MessageBoxButtons Buttons, MessageBoxIcon Icon, MessageBoxDefaultButton DefaultButton)
	{
		return ShowCore(Text, Caption, Buttons, Icon, DefaultButton, (MessageBoxOptions)0, false, Owner);
	}

	DialogResult MessageBox::Show(Control* Owner, const string& Text, const string& Caption, MessageBoxButtons Buttons, MessageBoxIcon Icon, MessageBoxDefaultButton DefaultButton, MessageBoxOptions Options)
	{
		return ShowCore(Text, Caption, Buttons, Icon, DefaultButton, Options, false, Owner);
	}

	void MessageBox::SetMessageBoxColors(Drawing::Color Foreground, Drawing::Color Background, Drawing::Color Bottom)
	{
		ForegroundColor = Foreground;
		BackgroundColor = Background;
		BottomColor = Bottom;
	}
}
