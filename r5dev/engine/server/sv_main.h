#pragma once
#include "networksystem/pylon.h"
#include "public/include/bansystem.h"

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

void SV_IsClientBanned(CPylon* pPylon, const string svIPAddr, uint64_t nNucleusID);
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
class HSV_Main : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: SV_InitGameDLL                       : {:#18x} |\n", p_SV_InitGameDLL.GetPtr());
		spdlog::debug("| FUN: SV_ShutdownGameDLL                   : {:#18x} |\n", p_SV_ShutdownGameDLL.GetPtr());
		spdlog::debug("| FUN: SV_CreateBaseline                    : {:#18x} |\n", p_SV_CreateBaseline.GetPtr());
		spdlog::debug("| FUN: CGameServer::SpawnServer             : {:#18x} |\n", p_CGameServer__SpawnServer.GetPtr());
		spdlog::debug("| VAR: s_bDedicated                         : {:#18x} |\n", reinterpret_cast<uintptr_t>(s_bDedicated));
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		p_SV_InitGameDLL     = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x81\xEC\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x80\x3D\x00\x00\x00\x00\x00\x0F\x85\x00\x00\x00\x00"), "xxx????x????xx?????xx????");
		p_SV_ShutdownGameDLL = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x83\xEC\x28\x80\x3D\x00\x00\x00\x00\x00\x0F\x84\x00\x00\x00\x00\x48\x8B\x0D\x00\x00\x00\x00\x48"), "xxxxxx?????xx????xxx????x");
		p_SV_CreateBaseline  = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x83\xEC\x28\x48\x8B\x0D\x00\x00\x00\x00\x48\x85\xC9\x75\x07"), "xxxxxxx????xxxxx");
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CGameServer__SpawnServer = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x55\x56\x57\x41\x55\x41\x56\x41\x57\x48\x81\xEC\x00\x00\x00\x00"), "xxxxxxxxxxxxxx????");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_CGameServer__SpawnServer = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\xC4\x53\x55\x56\x57\x41\x54\x41\x55\x41\x57"), "xxxxxxxxxxxxx");
		// 0x140312D80 // 48 8B C4 53 55 56 57 41 54 41 55 41 57 //
#endif
		SV_InitGameDLL           = p_SV_InitGameDLL.RCast<void(*)(void)>();
		SV_ShutdownGameDLL       = p_SV_ShutdownGameDLL.RCast<void(*)(void)>();
		SV_CreateBaseline        = p_SV_CreateBaseline.RCast<bool(*)(void)>();
		CGameServer__SpawnServer = p_CGameServer__SpawnServer.RCast<bool(*)(void*, const char*, const char*)>();
	}
	virtual void GetVar(void) const
	{
		s_bDedicated = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>(
			"\x48\x89\x4C\x24\x00\x48\x89\x54\x24\x00\x4C\x89\x44\x24\x00\x4C\x89\x4C\x24\x00\x53\x57\xB8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x2B\xE0\x48\x8B\xD9\x48\x8D\xBC\x24\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x89\x7C\x24\x00\x48\x8D\x54\x24\x00\x33\xFF"),
			"xxxx?xxxx?xxxx?xxxx?xxx????x????xxxxxxxxxx????x????xxxx?xxxx?xx").FindPatternSelf("40 38 3D", CMemory::Direction::DOWN).ResolveRelativeAddressSelf(0x3, 0x7).RCast<bool*>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HSV_Main);
