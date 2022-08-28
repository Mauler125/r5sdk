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


	static bool ModInit(CEngineAPI* pEngineAPI, const char* pModName, const char* pGameDir);
private:
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

inline CMemory p_PakFile_Init;
inline auto PakFile_Init = p_PakFile_Init.RCast<void (*)(char* buffer, char* source, char vpk_file)>();

inline CMemory p_ResetMTVFTaskItem;
inline auto v_ResetMTVFTaskItem = p_ResetMTVFTaskItem.RCast<void*(*)(void)>();

inline int64_t* g_pMTVFTaskItem; // struct.
inline char* g_szMTVFItemName;


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
		spdlog::debug("| FUN: PakFile_Init                         : {:#18x} |\n", p_PakFile_Init.GetPtr());
		spdlog::debug("| FUN: ResetMTVFTaskItem                    : {:#18x} |\n", p_ResetMTVFTaskItem.GetPtr());
		spdlog::debug("| VAR: g_pMTVFTaskItem                      : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_pMTVFTaskItem));
		spdlog::debug("| VAR: g_szMTVFItemName                     : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_szMTVFItemName));
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		p_CEngineAPI_Connect = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x83\xEC\x28\x48\x8B\x05\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00\x48\x85\xC0\x48\x89\x15"), "xxxxxxx????xxx????xxxxxx");
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CEngineAPI_ModInit  = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x81\xEC\x00\x00\x00\x00\x4D\x8B\xF0"), "xxxx?xxxx?xxxx?xxxxxxxxxxxx????xxx");
		p_CEngineAPI_MainLoop = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x55\x48\x81\xEC\x00\x00\x00\x00\x45\x33\xC9"), "xxxx?xxxx????xxx");
		p_PakFile_Init        = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x44\x88\x44\x24\x00\x56\x57\x41\x54\x41\x56\x41\x57\x48\x83\xEC\x20"), "xxxx?xxxx?xxxx?xxxxxxxxxxxx");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_CEngineAPI_ModInit  = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x4C\x24\x00\x55\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x81\xEC\x00\x00\x00\x00\x4D\x8B\xF8"), "xxxx?xxxx?xxxxxxxxxxxxxx????xxx");
		p_CEngineAPI_MainLoop = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\xE8\x00\x00\x00\x00\x48\x8B\x15\x00\x00\x00\x00\x84\xC0\xB9\x00\x00\x00\x00"), "x????xxx????xxx????").FollowNearCallSelf();
		p_PakFile_Init        = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x44\x88\x44\x24\x00\x53\x55\x56\x57"), "xxxx?xxxx");
#endif
		p_ResetMTVFTaskItem   = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x83\xEC\x28\x48\x8B\x15\x00\x00\x00\x00\x48\x85\xD2\x0F\x84\x00\x00\x00\x00\x48\x8B\x0D\x00\x00\x00\x00\x48\x8B\x01\xFF\x90\x00\x00\x00\x00\x33\xC9\xE8\x00\x00\x00\x00\x0F\x28\x05\x00\x00\x00\x00\x0F\x28\x0D\x00\x00\x00\x00\x0F\x11\x05\x00\x00\x00\x00\x0F\x28\x05\x00\x00\x00\x00\x0F\x11\x0D\x00\x00\x00\x00\x0F\x28\x0D\x00\x00\x00\x00\x0F\x11\x05\x00\x00\x00\x00\x0F\x11\x0D\x00\x00\x00\x00\x48\xC7\x05\x00\x00\x00\x00\x00\x00\x00\x00\xFF\x15\x00\x00\x00\x00"),
			"xxxxxxx????xxxxx????xxx????xxxxx????xxx????xxx????xxx????xxx????xxx????xxx????xxx????xxx????xxx????xxx????????xx????");

		CEngineAPI_Connect  = p_CEngineAPI_Connect.RCast<bool (*)(CEngineAPI*, CreateInterfaceFn)>();        /*48 83 EC 28 48 8B 05 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? 48 85 C0 48 89 15 ?? ?? ?? ??*/
		CEngineAPI_ModInit  = p_CEngineAPI_ModInit.RCast<bool (*)(CEngineAPI*, const char*, const char*)>(); /*48 89 5C 24 ?? 48 89 4C 24 ?? 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ?? ?? ?? ?? 4D 8B F8*/
		CEngineAPI_MainLoop = p_CEngineAPI_MainLoop.RCast<bool(*)(void)>();                                  /*E8 ?? ?? ?? ?? 48 8B 15 ?? ?? ?? ?? 84 C0 B9 ?? ?? ?? ??*/
		PakFile_Init = p_PakFile_Init.RCast<void (*)(char*, char*, char)>();                                 /*44 88 44 24 ?? 53 55 56 57*/
	}
	virtual void GetVar(void) const
	{
		g_pMTVFTaskItem = p_ResetMTVFTaskItem.FindPattern("48 8B", CMemory::Direction::DOWN, 250).ResolveRelativeAddressSelf(0x3, 0x7).RCast<int64_t*>();
		g_szMTVFItemName = p_ResetMTVFTaskItem.FindPattern("C6 05", CMemory::Direction::DOWN, 250).ResolveRelativeAddressSelf(0x2, 0x7).RCast<char*>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VSys_Dll2);