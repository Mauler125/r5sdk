#pragma once
#include "vpc/interfaces.h"
#include "common/engine_launcher_api.h"

class CEngineAPI : public IEngineAPI
{
public:
	virtual bool Connect(CreateInterfaceFn factory) = 0;
	virtual void Disconnect() = 0;
	virtual void* QueryInterface(const char* pInterfaceName) = 0;
	virtual InitReturnVal_t Init() = 0;
	virtual void Shutdown() = 0;
	virtual AppSystemTier_t GetTier() = 0;
	virtual void Reconnect(CreateInterfaceFn factory, const char* pInterfaceName) = 0;

	// This function must be called before init
	virtual bool SetStartupInfo(StartupInfo_t& info) = 0;

	virtual int Run() = 0;

	// Posts a console command
	virtual void PostConsoleCommand(const char* pConsoleCommand) = 0;

	// Are we running the simulation?
	virtual bool IsRunningSimulation() const = 0;

	// Start/stop running the simulation
	virtual void ActivateSimulation(bool bActive) = 0;

	// Reset the map we're on
	virtual void SetMap(const char* pMapName) = 0;

	static InitReturnVal_t VInit(CEngineAPI* thisp);
	static bool VModInit(CEngineAPI* pEngineAPI, const char* pModName, const char* pGameDir);
	static void VSetStartupInfo(CEngineAPI* pEngineAPI, StartupInfo_t* pStartupInfo);

	static void PumpMessages();
	static bool MainLoop();
//private:
	void* m_hEditorHWnd;
	bool m_bRunningSimulation;
	StartupInfo_t m_StartupInfo;
};

inline CMemory p_CEngineAPI_Init;
inline InitReturnVal_t(*CEngineAPI_Init)(CEngineAPI* thisp);

inline CMemory p_CEngineAPI_Shutdown;
inline void(*CEngineAPI_Shutdown)(void);

inline CMemory p_CEngineAPI_Connect;
inline bool(*CEngineAPI_Connect)(CEngineAPI* thisptr, CreateInterfaceFn factory);

inline CMemory p_CEngineAPI_ModInit;
inline bool(*CEngineAPI_ModInit)(CEngineAPI* pEngineAPI, const char* pModName, const char* pGameDir);

inline CMemory p_CEngineAPI_MainLoop;
inline bool(*CEngineAPI_MainLoop)(void);

inline CMemory p_CEngineAPI_PumpMessages;
inline void(*CEngineAPI_PumpMessages)(void);

inline CMemory p_CEngineAPI_SetStartupInfo;
inline void(*v_CEngineAPI_SetStartupInfo)(CEngineAPI* pEngineAPI, StartupInfo_t* pStartupInfo);

inline CMemory p_ResetMTVFTaskItem;
inline void*(*v_ResetMTVFTaskItem)(void);

inline CMemory p_PakFile_Init;
inline void(*PakFile_Init)(char* buffer, char* source, char vpk_file);

inline bool* g_bTextMode = nullptr;
inline char* g_szBaseDir = nullptr; // static size = 260
inline int64_t* g_pMTVFTaskItem = nullptr; // struct.
inline char* g_szMTVFItemName = nullptr;

///////////////////////////////////////////////////////////////////////////////
class VSys_Dll2 : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CEngineAPI::Init", p_CEngineAPI_Init.GetPtr());
		LogFunAdr("CEngineAPI::Shutdown", p_CEngineAPI_Shutdown.GetPtr());
		LogFunAdr("CEngineAPI::Connect", p_CEngineAPI_Connect.GetPtr());
		LogFunAdr("CEngineAPI::ModInit", p_CEngineAPI_ModInit.GetPtr());
		LogFunAdr("CEngineAPI::MainLoop", p_CEngineAPI_MainLoop.GetPtr());
		LogFunAdr("CEngineAPI::PumpMessages", p_CEngineAPI_PumpMessages.GetPtr());
#if defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		LogFunAdr("CEngineAPI::SetStartupInfo", p_CEngineAPI_SetStartupInfo.GetPtr());
#endif
		LogFunAdr("ResetMTVFTaskItem", p_ResetMTVFTaskItem.GetPtr());
		LogFunAdr("PakFile_Init", p_PakFile_Init.GetPtr());
		LogVarAdr("g_bTextMode", reinterpret_cast<uintptr_t>(g_bTextMode));
		LogVarAdr("g_szBaseDir", reinterpret_cast<uintptr_t>(g_szBaseDir));
		LogVarAdr("g_pMTVFTaskItem", reinterpret_cast<uintptr_t>(g_pMTVFTaskItem));
		LogVarAdr("g_szMTVFItemName", reinterpret_cast<uintptr_t>(g_szMTVFItemName));
	}
	virtual void GetFun(void) const
	{
		p_CEngineAPI_Init    = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 48 8B F1 48 8D 3D ?? ?? ?? ?? 33 DB 48 8D 15 ?? ?? ?? ??");
		p_CEngineAPI_Connect = g_GameDll.FindPatternSIMD("48 83 EC 28 48 8B 05 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? 48 85 C0 48 89 15");
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CEngineAPI_Shutdown = g_GameDll.FindPatternSIMD("41 54 41 56 48 83 EC 38 48 8B 0D ?? ?? ?? ??");
		p_CEngineAPI_ModInit  = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 41 54 41 55 41 56 41 57 48 81 EC ?? ?? ?? ?? 4D 8B F0");
		p_CEngineAPI_MainLoop = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 55 48 81 EC ?? ?? ?? ?? 45 33 C9");
		p_PakFile_Init        = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 44 88 44 24 ?? 56 57 41 54 41 56 41 57 48 83 EC 20");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_CEngineAPI_Shutdown = g_GameDll.FindPatternSIMD("48 83 EC 28 48 8B 0D ?? ?? ?? ?? 33 D2 48 8B 01 FF 90 ?? ?? ?? ?? B1 01");
		p_CEngineAPI_ModInit  = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 4C 24 ?? 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ?? ?? ?? ?? 4D 8B F8");
		p_CEngineAPI_MainLoop = g_GameDll.FindPatternSIMD("4C 8B DC 49 89 4B 08 48 81 EC ?? ?? ?? ?? 8B 05 ?? ?? ?? ??");
		p_PakFile_Init        = g_GameDll.FindPatternSIMD("44 88 44 24 ?? 53 55 56 57");
#endif
		p_CEngineAPI_PumpMessages = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 55 48 81 EC ?? ?? ?? ?? 45 33 C9");
		p_CEngineAPI_SetStartupInfo = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? ?? 48 81 EC ?? ?? ?? ?? 80 3D ?? ?? ?? ?? ?? 48 8B DA");
		p_ResetMTVFTaskItem = g_GameDll.FindPatternSIMD("48 83 EC 28 48 8B 15 ?? ?? ?? ?? 48 85 D2 0F 84 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 48 8B 01 FF 90 ?? ?? ?? ?? 33 C9 E8 ?? ?? ?? ?? 0F 28 05 ?? ?? ?? ?? 0F 28 0D ?? ?? ?? ?? 0F 11 05 ?? ?? ?? ?? 0F 28 05 ?? ?? ?? ?? 0F 11 0D ?? ?? ?? ?? 0F 28 0D ?? ?? ?? ?? 0F 11 05 ?? ?? ?? ?? 0F 11 0D ?? ?? ?? ?? 48 C7 05 ?? ?? ?? ?? ?? ?? ?? ?? FF 15 ?? ?? ?? ??");

		CEngineAPI_Init     = p_CEngineAPI_Init.RCast<InitReturnVal_t(*)(CEngineAPI*)>();
		CEngineAPI_Shutdown = p_CEngineAPI_Shutdown.RCast<void (*)(void)>();
		CEngineAPI_Connect  = p_CEngineAPI_Connect.RCast<bool (*)(CEngineAPI*, CreateInterfaceFn)>();             /*48 83 EC 28 48 8B 05 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? 48 85 C0 48 89 15 ?? ?? ?? ??*/
		CEngineAPI_ModInit  = p_CEngineAPI_ModInit.RCast<bool (*)(CEngineAPI*, const char*, const char*)>();      /*48 89 5C 24 ?? 48 89 4C 24 ?? 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ?? ?? ?? ?? 4D 8B F8*/
		CEngineAPI_MainLoop = p_CEngineAPI_MainLoop.RCast<bool(*)(void)>();                                       /*E8 ?? ?? ?? ?? 48 8B 15 ?? ?? ?? ?? 84 C0 B9 ?? ?? ?? ??*/
		CEngineAPI_PumpMessages = p_CEngineAPI_PumpMessages.RCast<void(*)(void)>();
		v_CEngineAPI_SetStartupInfo = p_CEngineAPI_SetStartupInfo.RCast<void (*)(CEngineAPI*, StartupInfo_t*)>(); /*48 89 5C 24 ?? 57 48 81 EC ?? ?? ?? ?? 80 3D ?? ?? ?? ?? ?? 48 8B DA*/
		PakFile_Init = p_PakFile_Init.RCast<void (*)(char*, char*, char)>();                                      /*44 88 44 24 ?? 53 55 56 57*/
	}
	virtual void GetVar(void) const
	{
		g_bTextMode = p_CEngineAPI_SetStartupInfo.FindPattern("80 3D", CMemory::Direction::DOWN, 250).ResolveRelativeAddressSelf(0x2, 0x7).RCast<bool*>();
		g_szBaseDir = p_CEngineAPI_SetStartupInfo.FindPattern("48 8D", CMemory::Direction::DOWN, 250).ResolveRelativeAddressSelf(0x3, 0x7).RCast<char*>();

		g_pMTVFTaskItem = p_ResetMTVFTaskItem.FindPattern("48 8B", CMemory::Direction::DOWN, 250).ResolveRelativeAddressSelf(0x3, 0x7).RCast<int64_t*>();
		g_szMTVFItemName = p_ResetMTVFTaskItem.FindPattern("C6 05", CMemory::Direction::DOWN, 250).ResolveRelativeAddressSelf(0x2, 0x7).RCast<char*>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
