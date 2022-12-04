#pragma once
#ifndef DEDICATED
#include "launcher/IApplication.h"
#endif // !DEDICATED
#include "public/globalvars_base.h"
#ifndef CLIENT_DLL
#include "engine/server/sv_main.h"
#endif // !CLIENT_DLL


//-----------------------------------------------------------------------------
// Purpose: Defines the ways that a map can be loaded.
// Note   : Only seems to get set and checked on 0 and 2: [r5apex_ds.exe + d55990d] for more details.
//-----------------------------------------------------------------------------
enum MapLoadType_t
{
	MapLoad_NewGame    = 0,
	MapLoad_Background = 2
};

//-----------------------------------------------------------------------------
// Purpose: Global variables shared between the engine and the game .dll
//-----------------------------------------------------------------------------
class CGlobalVars : public CGlobalVarsBase
{
public:
	// Current map
	const char*     m_pszMapName;
	int             m_nMapVersion;
	char            m_pad0[4];
	const char*     m_pszStartSpot;   // Seems empty at all times.
	MapLoadType_t   m_eLoadType;      // How the current map was loaded.
	bool            m_bMapLoadFailed; // Map has failed to load, we need to kick back to the main menu (unused?).

	void*           m_pUnk0;          // r5apex_ds.exe 'CBaseServer::Clear() + 0x7E'
	void*           m_pUnk1;          // r5apex_ds.exe 'CBaseServer::Clear() + 0x93'
	void*           m_pUnk2;          // r5apex_ds.exe 'CServer::FrameJob()  + 0x20'
	void*           m_pUnk3;
}; // Size 0x0098

#ifndef CLIENT_DLL
inline CGlobalVars* g_ServerGlobalVariables = nullptr;
#endif // !CLIENT_DLL
#ifndef DEDICATED
inline CGlobalVarsBase* g_ClientGlobalVariables = nullptr;
#endif // !DEDICATED

///////////////////////////////////////////////////////////////////////////////
class VEdict : public IDetour
{
	virtual void GetAdr(void) const
	{
#ifndef CLIENT_DLL
		spdlog::debug("| VAR: g_ServerGlobalVariables              : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_ServerGlobalVariables));
#endif // !CLIENT_DLL
#ifndef DEDICATED
		spdlog::debug("| VAR: g_ClientGlobalVariables              : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_ClientGlobalVariables));
#endif // !DEDICATED
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const
	{
#ifndef CLIENT_DLL
		g_ServerGlobalVariables = g_GameDll.FindPatternSIMD("48 81 EC ?? ?? ?? ?? E8 ?? ?? ?? ?? 80 3D ?? ?? ?? ?? ?? 0F 85 ?? ?? ?? ??")
			.FindPatternSelf("48 8D ?? ?? ?? ?? 01", CMemory::Direction::DOWN).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CGlobalVars*>();
#endif // !CLIENT_DLL
#ifndef DEDICATED
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		g_ClientGlobalVariables = g_GameDll.FindPatternSIMD("48 8B C4 57 41 54 41 55 41 56 41 57 48 83 EC 60 48 C7 40 ?? ?? ?? ?? ?? 48 89 58 08")
			.FindPatternSelf("4C 8D ?? ?? ?? ?? 01", CMemory::Direction::DOWN, 8000).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CGlobalVarsBase*>();
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		g_ClientGlobalVariables = g_GameDll.FindPatternSIMD("48 8B C4 55 41 54 41 55 41 56 41 57 48 8B EC 48 83 EC 60")
			.FindPatternSelf("4C 8D ?? ?? ?? ?? 01", CMemory::Direction::DOWN, 8000).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CGlobalVarsBase*>();
#endif // GAME_DLL
#endif // !DEDICATED
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VEdict);
