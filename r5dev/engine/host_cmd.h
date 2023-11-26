#pragma once
#include "tier1/cmd.h"
#include "sys_dll.h"

//-------------------------------------------------------------------------------------
// Forward declarations
//-------------------------------------------------------------------------------------
class CClient;

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
inline void*(*v_Host_Init)(bool* bDedicated);

inline CMemory p_Host_Shutdown;
inline void(*v_Host_Shutdown)();

inline CMemory p_Host_NewGame;
inline bool(*v_Host_NewGame)(char* pszMapName, char* pszMapGroup, bool bLoadGame, char bBackground, LARGE_INTEGER PerformanceCount);

inline CMemory p_Host_Disconnect;
inline void(*v_Host_Disconnect)(bool bShowMainMenu);

inline CMemory p_Host_ChangeLevel;
inline bool(*v_Host_ChangeLevel)(bool bLoadFromSavedGame, const char* pszMapName, const char* pszMapGroup);

inline CMemory p_Host_Status_PrintClient;
inline void (*v_Host_Status_PrintClient)(CClient* client, bool bShowAddress, void (*print) (const char* fmt, ...));

inline CMemory p_SetLaunchOptions;
inline int(*v_SetLaunchOptions)(const CCommand& args);

#if !defined (GAMEDLL_S0) && !defined (GAMEDLL_S1) && !defined (GAMEDLL_S2)
inline CMemory p_DFS_InitializeFeatureFlagDefinitions;
inline bool(*v_DFS_InitializeFeatureFlagDefinitions)(const char* pszFeatureFlags);
#endif // !(GAMEDLL_S0) || !(GAMEDLL_S1) || !(GAMEDLL_S2)

extern EngineParms_t* g_pEngineParms;

///////////////////////////////////////////////////////////////////////////////
class VHostCmd : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("Host_Init", p_Host_Init.GetPtr());
		LogFunAdr("Host_Shutdown", p_Host_Shutdown.GetPtr());
		LogFunAdr("Host_Disconnect", p_Host_Disconnect.GetPtr());
		LogFunAdr("Host_NewGame", p_Host_NewGame.GetPtr());
		LogFunAdr("Host_ChangeLevel", p_Host_ChangeLevel.GetPtr());
		LogFunAdr("Host_Status_PrintClient", p_Host_Status_PrintClient.GetPtr());
		LogFunAdr("SetLaunchOptions", p_SetLaunchOptions.GetPtr());
#if !defined (GAMEDLL_S0) && !defined (GAMEDLL_S1) && !defined (GAMEDLL_S2)
		LogFunAdr("DFS_InitializeFeatureFlagDefinitions", p_DFS_InitializeFeatureFlagDefinitions.GetPtr());
#endif // !(GAMEDLL_S0) || !(GAMEDLL_S1) || !(GAMEDLL_S2)
		LogVarAdr("g_pEngineParms", reinterpret_cast<uintptr_t>(g_pEngineParms));
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_Host_Init = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 41 54 41 55 41 56 41 57 48 81 EC ?? ?? ?? ?? 48 8B D9 FF 15 ?? ?? ?? ??");
		p_Host_NewGame = g_GameDll.FindPatternSIMD("48 8B C4 56 41 54 41 57 48 81 EC ?? ?? ?? ?? F2 0F 10 05 ?? ?? ?? ??");
		p_Host_Disconnect = g_GameDll.FindPatternSIMD("48 83 EC 38 48 89 7C 24 ?? 0F B6 F9");
		p_Host_ChangeLevel = g_GameDll.FindPatternSIMD("40 53 56 41 56 48 81 EC ?? ?? ?? ?? 49 8B D8");
		p_SetLaunchOptions = g_GameDll.FindPatternSIMD("48 89 6C 24 ?? 57 48 83 EC 20 48 8B E9 48 8B 0D ?? ?? ?? ??");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_Host_Init = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 55 41 54 41 55 41 56 41 57 48 8D AC 24 ?? ?? ?? ?? B8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 2B E0 48 8B D9");
		p_Host_NewGame = g_GameDll.FindPatternSIMD("48 8B C4 ?? 41 54 41 55 48 81 EC 70 04 ?? ?? F2 0F 10 05 ?? ?? ?? 0B");
		p_Host_Disconnect = g_GameDll.FindPatternSIMD("40 53 48 83 EC 30 0F B6 D9");
		p_Host_ChangeLevel = g_GameDll.FindPatternSIMD("40 56 57 41 56 48 81 EC ?? ?? ?? ??");
		p_SetLaunchOptions = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 57 48 83 EC 20 48 8B 1D ?? ?? ?? ?? 48 8B E9 48 85 DB");
#endif
		p_Host_Shutdown = g_GameDll.FindPatternSIMD("48 8B C4 48 83 EC ?? 80 3D ?? ?? ?? ?? ?? 0F 85 ?? ?? ?? ?? 8B 15 ?? ?? ?? ??");
		p_Host_Status_PrintClient = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 56 57 41 56 48 83 EC 60 48 8B A9 ?? ?? ?? ??");
#if !defined (GAMEDLL_S0) && !defined (GAMEDLL_S1) && !defined (GAMEDLL_S2)
		p_DFS_InitializeFeatureFlagDefinitions = g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 40 38 3D ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? 48 8B CE").FollowNearCallSelf();
		v_DFS_InitializeFeatureFlagDefinitions = p_DFS_InitializeFeatureFlagDefinitions.RCast<bool (*)(const char*)>(); /*48 8B C4 55 53 48 8D 68 E8*/
#endif // !(GAMEDLL_S0) || !(GAMEDLL_S1) || !(GAMEDLL_S2)
		v_Host_Init = p_Host_Init.RCast<void* (*)(bool*)>();                                        /*48 89 5C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 55 41 54 41 55 41 56 41 57 48 8D AC 24 ?? ?? ?? ?? B8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 2B E0 48 8B D9*/
		v_Host_Shutdown = p_Host_Shutdown.RCast<void (*)()>();
		v_Host_NewGame = p_Host_NewGame.RCast<bool (*)(char*, char*, bool, char, LARGE_INTEGER)>(); /*48 8B C4 ?? 41 54 41 55 48 81 EC 70 04 00 00 F2 0F 10 05 ?? ?? ?? 0B*/
		v_Host_Disconnect = p_Host_Disconnect.RCast<void (*)(bool)>();
		v_Host_ChangeLevel = p_Host_ChangeLevel.RCast<bool (*)(bool, const char*, const char*)>();  /*40 56 57 41 56 48 81 EC ?? ?? ?? ??*/
		v_Host_Status_PrintClient = p_Host_Status_PrintClient.RCast<void (*)(CClient*, bool, void (*) (const char*, ...))>();
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
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
