#pragma once
#include "tier1/cmd.h"
#include "launcher/IApplication.h"

struct EngineParms_t
{
	char* baseDirectory;
	char* modName;
	char* rootGameName;
	unsigned int memSizeAvailable;
};

extern EngineParms_t* g_pEngineParms;

/* ==== HOST ============================================================================================================================================================ */
inline CMemory p_Host_Init;
inline auto Host_Init = p_Host_Init.RCast<void* (*)(bool* bDedicated)>();

inline CMemory p_Host_NewGame;
inline auto Host_NewGame = p_Host_NewGame.RCast<bool (*)(char* pszMapName, char* pszMapGroup, bool bLoadGame, char bBackground, LARGE_INTEGER PerformanceCount)>();

inline CMemory p_Host_ChangeLevel;
inline auto Host_ChangeLevel = p_Host_ChangeLevel.RCast<bool (*)(bool bLoadFromSavedGame, const char* pszMapName, const char* pszMapGroup)>();

inline CMemory p_SetLaunchOptions;
inline auto v_SetLaunchOptions = p_SetLaunchOptions.RCast<int (*)(const CCommand& args)>();

extern EngineParms_t* g_pEngineParms;


///////////////////////////////////////////////////////////////////////////////
class VHostCmd : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: Host_Init                            : {:#18x} |\n", p_Host_Init.GetPtr());
		spdlog::debug("| FUN: Host_NewGame                         : {:#18x} |\n", p_Host_NewGame.GetPtr());
		spdlog::debug("| FUN: Host_ChangeLevel                     : {:#18x} |\n", p_Host_ChangeLevel.GetPtr());
		spdlog::debug("| FUN: SetLaunchOptions                     : {:#18x} |\n", p_SetLaunchOptions.GetPtr());
		spdlog::debug("| VAR: g_pEngineParms                       : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_pEngineParms));
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_Host_Init = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\xD9\xFF\x15\x00\x00\x00\x00"), "xxxx?xxxx?xxxx?xxxxxxxxxxxx????xxxxx????");
		p_Host_NewGame = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\xC4\x56\x41\x54\x41\x57\x48\x81\xEC\x00\x00\x00\x00\xF2\x0F\x10\x05\x00\x00\x00\x00"), "xxxxxxxxxxx????xxxx????"); /*48 8B C4 56 41 54 41 57 48 81 EC ? ? ? ? F2 0F 10 05 ? ? ? ?*/
		p_Host_ChangeLevel = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x56\x41\x56\x48\x81\xEC\x00\x00\x00\x00\x49\x8B\xD8"), "xxxxxxxx????xxx");
		p_SetLaunchOptions = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x6C\x24\x00\x57\x48\x83\xEC\x20\x48\x8B\xE9\x48\x8B\x0D\x00\x00\x00\x00"), "xxxx?xxxxxxxxxxx????");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_Host_Init = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\xAC\x24\x00\x00\x00\x00\xB8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x2B\xE0\x48\x8B\xD9"), "xxxx?xxxx?xxxx?xxxxxxxxxxxxx????x????x????xxxxxx");
		p_Host_NewGame = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\xC4\x00\x41\x54\x41\x55\x48\x81\xEC\x70\x04\x00\x00\xF2\x0F\x10\x05\x00\x00\x00\x0B"), "xxx?xxxxxxxxxxxxxxx???x");
		p_Host_ChangeLevel = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x56\x57\x41\x56\x48\x81\xEC\x00\x00\x00\x00"), "xxxxxxxx????");
		p_SetLaunchOptions = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x57\x48\x83\xEC\x20\x48\x8B\x1D\x00\x00\x00\x00\x48\x8B\xE9\x48\x85\xDB"), "xxxx?xxxx?xxxxxxxx????xxxxxx");
#endif
		Host_Init = p_Host_Init.RCast<void* (*)(bool*)>();                                        /*48 89 5C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 55 41 54 41 55 41 56 41 57 48 8D AC 24 ?? ?? ?? ?? B8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 2B E0 48 8B D9*/
		Host_NewGame = p_Host_NewGame.RCast<bool (*)(char*, char*, bool, char, LARGE_INTEGER)>(); /*48 8B C4 ?? 41 54 41 55 48 81 EC 70 04 00 00 F2 0F 10 05 ?? ?? ?? 0B*/
		Host_ChangeLevel = p_Host_ChangeLevel.RCast<bool (*)(bool, const char*, const char*)>();  /*40 56 57 41 56 48 81 EC ?? ?? ?? ??*/
		v_SetLaunchOptions = p_SetLaunchOptions.RCast<int (*)(const CCommand&)>();                /*48 89 5C 24 ?? 48 89 6C 24 ?? 57 48 83 EC 20 48 8B 1D ?? ?? ?? ?? 48 8B E9 48 85 DB*/
	}
	virtual void GetVar(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		g_pEngineParms = p_CModAppSystemGroup_Main.FindPattern("48 8B", CMemory::Direction::DOWN, 100).ResolveRelativeAddress(0x3, 0x7).RCast<EngineParms_t*>();
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		g_pEngineParms = p_CModAppSystemGroup_Main.FindPattern("4C 8B", CMemory::Direction::DOWN, 100).ResolveRelativeAddress(0x3, 0x7).RCast<EngineParms_t*>();
#endif
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VHostCmd);
