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

void SysGame_Attach();
void SysGame_Detach();

///////////////////////////////////////////////////////////////////////////////
class VGame : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: CGame::PlayStartupVideos             : {:#18x} |\n", p_CGame__PlayStartupVideos.GetPtr());
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CGame__PlayStartupVideos = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\xC4\x48\x81\xEC\x00\x00\x00\x00\x80\x3D\x00\x00\x00\x00\x00\x0F\x85\x00\x00\x00\x00\x48\x8B\x0D\x00\x00\x00\x00"), "xxxxxx????xx?????xx????xxx????");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_CGame__PlayStartupVideos = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\xC4\x55\x48\x8D\xA8\x00\x00\x00\x00\x48\x81\xEC\x00\x00\x00\x00\x80\x3D\x00\x00\x00\x00\x00"), "xxxxxxx????xxx????xx?????");
#endif
		v_CGame__PlayStartupVideos = p_CGame__PlayStartupVideos.RCast<void (*)(void)>();
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VGame);