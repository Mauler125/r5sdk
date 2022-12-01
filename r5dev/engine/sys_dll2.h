#pragma once
#include "vpc/interfaces.h"
#include "appframework/engine_launcher_api.h"

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


	static bool VModInit(CEngineAPI* pEngineAPI, const char* pModName, const char* pGameDir);
	static void VSetStartupInfo(CEngineAPI* pEngineAPI, StartupInfo_t* pStartupInfo);
//private:
	void* m_hEditorHWnd;
	bool m_bRunningSimulation;
	StartupInfo_t m_StartupInfo;
};

inline CMemory p_CEngineAPI_Connect;
inline auto CEngineAPI_Connect = p_CEngineAPI_Connect.RCast<bool (*)(CEngineAPI* thisptr, CreateInterfaceFn factory)>();

inline CMemory p_CEngineAPI_ModInit;
inline auto CEngineAPI_ModInit = p_CEngineAPI_ModInit.RCast<bool (*)(CEngineAPI* pEngineAPI, const char* pModName, const char* pGameDir)>();

inline CMemory p_CEngineAPI_MainLoop;
inline auto CEngineAPI_MainLoop = p_CEngineAPI_MainLoop.RCast<bool(*)(void)>();

inline CMemory p_CEngineAPI_SetStartupInfo;
inline auto v_CEngineAPI_SetStartupInfo = p_CEngineAPI_SetStartupInfo.RCast<void (*)(CEngineAPI* pEngineAPI, StartupInfo_t* pStartupInfo)>();

inline CMemory p_ResetMTVFTaskItem;
inline auto v_ResetMTVFTaskItem = p_ResetMTVFTaskItem.RCast<void*(*)(void)>();

inline CMemory p_PakFile_Init;
inline auto PakFile_Init = p_PakFile_Init.RCast<void (*)(char* buffer, char* source, char vpk_file)>();

inline bool* g_bTextMode = nullptr;
inline char* g_szBaseDir = nullptr; // static size = 260
inline int64_t* g_pMTVFTaskItem = nullptr; // struct.
inline char* g_szMTVFItemName = nullptr;


void SysDll2_Attach();
void SysDll2_Detach();
///////////////////////////////////////////////////////////////////////////////
class VSys_Dll2 : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: CEngineAPI::Connect                  : {:#18x} |\n", p_CEngineAPI_Connect.GetPtr());
		spdlog::debug("| FUN: CEngineAPI::ModInit                  : {:#18x} |\n", p_CEngineAPI_ModInit.GetPtr());
		spdlog::debug("| FUN: CEngineAPI::MainLoop                 : {:#18x} |\n", p_CEngineAPI_MainLoop.GetPtr());
#if defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		spdlog::debug("| FUN: CEngineAPI::SetStartupInfo           : {:#18x} |\n", p_CEngineAPI_SetStartupInfo.GetPtr());
#endif
		spdlog::debug("| FUN: ResetMTVFTaskItem                    : {:#18x} |\n", p_ResetMTVFTaskItem.GetPtr());
		spdlog::debug("| FUN: PakFile_Init                         : {:#18x} |\n", p_PakFile_Init.GetPtr());
		spdlog::debug("| VAR: g_bTextMode                          : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_bTextMode));
		spdlog::debug("| VAR: g_szBaseDir                          : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_szBaseDir));
		spdlog::debug("| VAR: g_pMTVFTaskItem                      : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_pMTVFTaskItem));
		spdlog::debug("| VAR: g_szMTVFItemName                     : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_szMTVFItemName));
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		p_CEngineAPI_Connect = g_GameDll.FindPatternSIMD("48 83 EC 28 48 8B 05 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? 48 85 C0 48 89 15");
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CEngineAPI_ModInit  = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 41 54 41 55 41 56 41 57 48 81 EC ?? ?? ?? ?? 4D 8B F0");
		p_CEngineAPI_MainLoop = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 55 48 81 EC ?? ?? ?? ?? 45 33 C9");
		p_PakFile_Init        = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 44 88 44 24 ?? 56 57 41 54 41 56 41 57 48 83 EC 20");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_CEngineAPI_ModInit  = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 4C 24 ?? 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ?? ?? ?? ?? 4D 8B F8");
		p_CEngineAPI_MainLoop = g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 48 8B 15 ?? ?? ?? ?? 84 C0 B9 ?? ?? ?? ??").FollowNearCallSelf();
		p_PakFile_Init        = g_GameDll.FindPatternSIMD("44 88 44 24 ?? 53 55 56 57");
#endif
		p_CEngineAPI_SetStartupInfo = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? ?? 48 81 EC ?? ?? ?? ?? 80 3D ?? ?? ?? ?? ?? 48 8B DA");
		p_ResetMTVFTaskItem = g_GameDll.FindPatternSIMD("48 83 EC 28 48 8B 15 ?? ?? ?? ?? 48 85 D2 0F 84 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 48 8B 01 FF 90 ?? ?? ?? ?? 33 C9 E8 ?? ?? ?? ?? 0F 28 05 ?? ?? ?? ?? 0F 28 0D ?? ?? ?? ?? 0F 11 05 ?? ?? ?? ?? 0F 28 05 ?? ?? ?? ?? 0F 11 0D ?? ?? ?? ?? 0F 28 0D ?? ?? ?? ?? 0F 11 05 ?? ?? ?? ?? 0F 11 0D ?? ?? ?? ?? 48 C7 05 ?? ?? ?? ?? ?? ?? ?? ?? FF 15 ?? ?? ?? ??");

		CEngineAPI_Connect  = p_CEngineAPI_Connect.RCast<bool (*)(CEngineAPI*, CreateInterfaceFn)>();             /*48 83 EC 28 48 8B 05 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? 48 85 C0 48 89 15 ?? ?? ?? ??*/
		CEngineAPI_ModInit  = p_CEngineAPI_ModInit.RCast<bool (*)(CEngineAPI*, const char*, const char*)>();      /*48 89 5C 24 ?? 48 89 4C 24 ?? 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ?? ?? ?? ?? 4D 8B F8*/
		CEngineAPI_MainLoop = p_CEngineAPI_MainLoop.RCast<bool(*)(void)>();                                       /*E8 ?? ?? ?? ?? 48 8B 15 ?? ?? ?? ?? 84 C0 B9 ?? ?? ?? ??*/
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
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VSys_Dll2);