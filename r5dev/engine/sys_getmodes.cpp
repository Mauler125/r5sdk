//===========================================================================//
//
// Purpose: 
//
//===========================================================================//
#include "core/stdafx.h"
#include "windows/id3dx.h"
#include "engine/sys_getmodes.h"

//-----------------------------------------------------------------------------
// Purpose: creates the game window, obtains the rect and plays the startup movie.
//-----------------------------------------------------------------------------
bool HCVideoMode_Common__CreateGameWindow(int* pnRect)
{
	g_nWindowWidth  = pnRect[0];
	g_nWindowHeight = pnRect[1];
	return CVideoMode_Common__CreateGameWindow(pnRect);
}

void HCVideoMode_Common_Attach()
{
	DetourAttach((LPVOID*)&CVideoMode_Common__CreateGameWindow, &HCVideoMode_Common__CreateGameWindow);
}

void HCVideoMode_Common_Detach()
{
	DetourDetach((LPVOID*)&CVideoMode_Common__CreateGameWindow, &HCVideoMode_Common__CreateGameWindow);
}
