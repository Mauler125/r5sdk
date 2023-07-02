//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
//===========================================================================//
#ifndef SYS_MAINWIND_H
#define SYS_MAINWIND_H
#include "public/igame.h"

inline CMemory p_CGame__AttachToWindow;
inline void (*v_CGame__AttachToWindow)(void);

inline CMemory p_CGame__PlayStartupVideos;
inline void(*v_CGame__PlayStartupVideos)(void);

inline CMemory p_CGame__WindowProc;
inline int(*v_CGame__WindowProc)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
//-----------------------------------------------------------------------------
// Purpose: Main game interface, including message pump and window creation
//-----------------------------------------------------------------------------
class CGame : public IGame
{
public:
	static void PlayStartupVideos(void);
	static int WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

inline HWND* g_pGameWindow = nullptr;

///////////////////////////////////////////////////////////////////////////////
class VGame : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CGame::AttachToWindow", p_CGame__AttachToWindow.GetPtr());
		LogFunAdr("CGame::PlayStartupVideos", p_CGame__PlayStartupVideos.GetPtr());
		LogVarAdr("g_GameWindow", reinterpret_cast<uintptr_t>(g_pGameWindow));
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CGame__AttachToWindow    = g_GameDll.FindPatternSIMD("48 83 EC 38 48 8B 0D ?? ?? ?? ?? 48 85 C9 0F 84 ?? ?? ?? ??");
		p_CGame__PlayStartupVideos = g_GameDll.FindPatternSIMD("48 8B C4 48 81 EC ?? ?? ?? ?? 80 3D ?? ?? ?? ?? ?? 0F 85 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ??");
		p_CGame__WindowProc = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 57 41 54 41 56 48 81 EC ?? ?? ?? ??");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_CGame__AttachToWindow    = g_GameDll.FindPatternSIMD("48 83 EC 28 48 8B 0D ?? ?? ?? ?? 48 85 C9 0F 84 ?? ?? ?? ?? BA ?? ?? ?? ??");
		p_CGame__PlayStartupVideos = g_GameDll.FindPatternSIMD("48 8B C4 55 48 8D A8 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 80 3D ?? ?? ?? ?? ??");
		p_CGame__WindowProc = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 55 41 56 41 57 48 8D 6C 24 ?? 48 81 EC ?? ?? ?? ?? 33 F6");
#endif

		v_CGame__AttachToWindow = p_CGame__AttachToWindow.RCast<void (*)(void)>();
		v_CGame__PlayStartupVideos = p_CGame__PlayStartupVideos.RCast<void (*)(void)>();
		v_CGame__WindowProc = p_CGame__WindowProc.RCast<int (*)(HWND, UINT, WPARAM, LPARAM)>();
	}
	virtual void GetVar(void) const
	{
		g_pGameWindow = p_CGame__AttachToWindow.FindPattern("48 8B 0D").ResolveRelativeAddressSelf(0x3, 0x7).RCast<HWND*>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const;
	virtual void Detach(void) const;
};
///////////////////////////////////////////////////////////////////////////////

#endif // SYS_MAINWIND_H