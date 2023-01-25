//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
//===========================================================================//
#include "public/igame.h"

inline CMemory p_CGame__PlayStartupVideos;
inline auto v_CGame__PlayStartupVideos = p_CGame__PlayStartupVideos.RCast<void (*)(void)>();
//-----------------------------------------------------------------------------
// Purpose: Main game interface, including message pump and window creation
//-----------------------------------------------------------------------------
class CGame : public IGame
{
public:
	static void PlayStartupVideos(void);
};

///////////////////////////////////////////////////////////////////////////////
class VGame : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CGame::PlayStartupVideos", p_CGame__PlayStartupVideos.GetPtr());
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CGame__PlayStartupVideos = g_GameDll.FindPatternSIMD("48 8B C4 48 81 EC ?? ?? ?? ?? 80 3D ?? ?? ?? ?? ?? 0F 85 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ??");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_CGame__PlayStartupVideos = g_GameDll.FindPatternSIMD("48 8B C4 55 48 8D A8 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 80 3D ?? ?? ?? ?? ??");
#endif
		v_CGame__PlayStartupVideos = p_CGame__PlayStartupVideos.RCast<void (*)(void)>();
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const;
	virtual void Detach(void) const;
};
///////////////////////////////////////////////////////////////////////////////
