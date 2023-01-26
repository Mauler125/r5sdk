#pragma once
#include "networksystem/pylon.h"
#include "networksystem/bansystem.h"

///////////////////////////////////////////////////////////////////////////////

/* ==== SV_MAIN ======================================================================================================================================================= */
inline CMemory p_SV_InitGameDLL;
inline auto SV_InitGameDLL = p_SV_InitGameDLL.RCast<void(*)(void)>();

inline CMemory p_SV_ShutdownGameDLL;
inline auto SV_ShutdownGameDLL = p_SV_ShutdownGameDLL.RCast<void(*)(void)>();

inline CMemory p_SV_CreateBaseline;
inline auto SV_CreateBaseline = p_SV_CreateBaseline.RCast<bool(*)(void)>();

inline CMemory p_CGameServer__SpawnServer;
inline auto CGameServer__SpawnServer = p_CGameServer__SpawnServer.RCast<bool(*)(void* thisptr, const char* pszMapName, const char* pszMapGroupName)>();

inline bool* s_bDedicated = nullptr;

///////////////////////////////////////////////////////////////////////////////

void SV_IsClientBanned(const string& svIPAddr, const uint64_t nNucleusID);
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
class HSV_Main : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CGameServer::SpawnServer", p_CGameServer__SpawnServer.GetPtr());
		LogFunAdr("SV_InitGameDLL", p_SV_InitGameDLL.GetPtr());
		LogFunAdr("SV_ShutdownGameDLL", p_SV_ShutdownGameDLL.GetPtr());
		LogFunAdr("SV_CreateBaseline", p_SV_CreateBaseline.GetPtr());
		LogVarAdr("s_bDedicated", reinterpret_cast<uintptr_t>(s_bDedicated));
	}
	virtual void GetFun(void) const
	{
		p_SV_InitGameDLL     = g_GameDll.FindPatternSIMD("48 81 EC ?? ?? ?? ?? E8 ?? ?? ?? ?? 80 3D ?? ?? ?? ?? ?? 0F 85 ?? ?? ?? ??");
		p_SV_ShutdownGameDLL = g_GameDll.FindPatternSIMD("48 83 EC 28 80 3D ?? ?? ?? ?? ?? 0F 84 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 48");
		p_SV_CreateBaseline  = g_GameDll.FindPatternSIMD("48 83 EC 28 48 8B 0D ?? ?? ?? ?? 48 85 C9 75 07");
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CGameServer__SpawnServer = g_GameDll.FindPatternSIMD("40 53 55 56 57 41 55 41 56 41 57 48 81 EC ?? ?? ?? ??");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_CGameServer__SpawnServer = g_GameDll.FindPatternSIMD("48 8B C4 53 55 56 57 41 54 41 55 41 57");
		// 0x140312D80 // 48 8B C4 53 55 56 57 41 54 41 55 41 57 //
#endif
		SV_InitGameDLL           = p_SV_InitGameDLL.RCast<void(*)(void)>();
		SV_ShutdownGameDLL       = p_SV_ShutdownGameDLL.RCast<void(*)(void)>();
		SV_CreateBaseline        = p_SV_CreateBaseline.RCast<bool(*)(void)>();
		CGameServer__SpawnServer = p_CGameServer__SpawnServer.RCast<bool(*)(void*, const char*, const char*)>();
	}
	virtual void GetVar(void) const
	{
		s_bDedicated = g_GameDll.FindPatternSIMD("48 89 4C 24 ?? 48 89 54 24 ?? 4C 89 44 24 ?? 4C 89 4C 24 ?? 53 57 B8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 2B E0 48 8B D9 48 8D BC 24 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 89 7C 24 ?? 48 8D 54 24 ?? 33 FF")
			.FindPatternSelf("40 38 3D", CMemory::Direction::DOWN).ResolveRelativeAddressSelf(0x3, 0x7).RCast<bool*>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////
