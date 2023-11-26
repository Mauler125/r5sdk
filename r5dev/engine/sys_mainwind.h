//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
//===========================================================================//
#ifndef SYS_MAINWIND_H
#define SYS_MAINWIND_H

inline CMemory p_CGame__AttachToWindow;
inline void (*v_CGame__AttachToWindow)(void);

inline CMemory p_CGame__PlayStartupVideos;
inline void(*v_CGame__PlayStartupVideos)(void);

inline CMemory p_CGame__WindowProc;
inline int(*v_CGame__WindowProc)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
//-----------------------------------------------------------------------------
// Purpose: Main game interface, including message pump and window creation
//-----------------------------------------------------------------------------
class CGame
{
public:
	static void PlayStartupVideos(void);
	static int WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


	inline HWND GetWindow() const { return m_hWindow; }
	void GetWindowRect(int* x, int* y, int* w, int* h);

	inline int GetDesktopWidth() const { return m_iDesktopWidth; }
	inline int GetDesktopHeight() const { return m_iDesktopHeight; }
	inline int GetDesktopRefreshRate() const { return m_iDesktopRefreshRate; }
	inline float GetTVRefreshRate() const // Avoid stutter on TV's running on broadcast frame rates.
	{ return ((float)m_iDesktopRefreshRate == 59.0f || (float)m_iDesktopRefreshRate == 60.0f) ? 59.939999f : (float)m_iDesktopRefreshRate; }

private:
	HWND m_hWindow;
	HINSTANCE m_hInstance;
	WNDPROC m_ChainedWindowProc;
	int m_x;
	int m_y;
	int m_width;
	int m_height;
	bool m_bUnk0;
	bool m_bUnk1;
	bool m_bExternallySuppliedWindow;
	int m_iDesktopWidth;
	int m_iDesktopHeight;
	int m_iDesktopRefreshRate;
	void* m_pInputContext_Maybe;
}; static_assert(sizeof(CGame) == 64);

inline CGame* g_pGame = nullptr;

///////////////////////////////////////////////////////////////////////////////
class VGame : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CGame::AttachToWindow", p_CGame__AttachToWindow.GetPtr());
		LogFunAdr("CGame::PlayStartupVideos", p_CGame__PlayStartupVideos.GetPtr());
		LogVarAdr("g_Game", reinterpret_cast<uintptr_t>(g_pGame));
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
		g_pGame = p_CGame__AttachToWindow.FindPattern("48 8B 0D").ResolveRelativeAddressSelf(0x3, 0x7).RCast<CGame*>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////

#endif // SYS_MAINWIND_H