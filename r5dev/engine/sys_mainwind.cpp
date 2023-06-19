//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
//===========================================================================//
#include "core/stdafx.h"
#include "tier0/commandline.h"
#include "windows/id3dx.h"
#include "windows/input.h"
#include "engine/sys_mainwind.h"
#include "gameui/IConsole.h"
#include "gameui/IBrowser.h"

//-----------------------------------------------------------------------------
// Purpose: plays the startup video's
//-----------------------------------------------------------------------------
void CGame::PlayStartupVideos(void)
{
	if (!CommandLine()->CheckParm("-novid"))
	{
		v_CGame__PlayStartupVideos();
	}
}

//-----------------------------------------------------------------------------
// Purpose: main windows procedure
//-----------------------------------------------------------------------------
int CGame::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (!g_bImGuiInitialized)
		return v_CGame__WindowProc(hWnd, uMsg, wParam, lParam);

	ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);

	if (uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN)
	{
		if (wParam == g_pImGuiConfig->m_ConsoleConfig.m_nBind0 || wParam == g_pImGuiConfig->m_ConsoleConfig.m_nBind1)
		{
			g_pConsole->m_bActivate ^= true;
			ResetInput(); // Disable input to game when console is drawn.
		}

		if (wParam == g_pImGuiConfig->m_BrowserConfig.m_nBind0 || wParam == g_pImGuiConfig->m_BrowserConfig.m_nBind1)
		{
			g_pBrowser->m_bActivate ^= true;
			ResetInput(); // Disable input to game when browser is drawn.
		}
	}

	if (g_pConsole->m_bActivate || g_pBrowser->m_bActivate)
	{//////////////////////////////////////////////////////////////////////////////
		g_bBlockInput = true;

		switch (uMsg)
		{
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_MBUTTONDBLCLK:
		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_MOUSEACTIVATE:
		case WM_MOUSEHOVER:
		case WM_MOUSEHWHEEL:
		case WM_MOUSELEAVE:
		case WM_MOUSEMOVE:
		case WM_MOUSEWHEEL:
		case WM_SETCURSOR:
			uMsg = WM_NULL;
			break;
		default:
			break;
		}
	}//////////////////////////////////////////////////////////////////////////////
	else
	{
		g_bBlockInput = false;
	}

	return v_CGame__WindowProc(hWnd, uMsg, wParam, lParam);
}

///////////////////////////////////////////////////////////////////////////////
void VGame::Attach() const
{
	DetourAttach(&v_CGame__PlayStartupVideos, &CGame::PlayStartupVideos);
	DetourAttach(&v_CGame__WindowProc, &CGame::WindowProc);
}

void VGame::Detach() const
{
	DetourDetach(&v_CGame__PlayStartupVideos, &CGame::PlayStartupVideos);
	DetourDetach(&v_CGame__WindowProc, &CGame::WindowProc);
}