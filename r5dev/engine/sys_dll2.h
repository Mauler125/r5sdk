#pragma once
#include "tier0/interface.h"

class CEngineAPI
{
public:
	static bool ModInit(CEngineAPI* pEngineAPI, const char* pModName, const char* pGameDir);
	// TODO [ AMOS ]:
};

inline CMemory p_CEngineAPI_Connect = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x83\xEC\x28\x48\x8B\x05\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00\x48\x85\xC0\x48\x89\x15"), "xxxxxxx????xxx????xxxxxx");
inline auto CEngineAPI_Connect = p_CEngineAPI_Connect.RCast<bool (*)(CEngineAPI* thisptr, CreateInterfaceFn factory)>(); /*48 83 EC 28 48 8B 05 ? ? ? ? 48 8D 0D ? ? ? ? 48 85 C0 48 89 15 ? ? ? ?*/

#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
inline CMemory p_PakFile_Init = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x44\x88\x44\x24\x00\x56\x57\x41\x54\x41\x56\x41\x57\x48\x83\xEC\x20"), "xxxx?xxxx?xxxx?xxxxxxxxxxxx");
inline auto PakFile_Init = p_PakFile_Init.RCast<void (*)(char* buffer, char* source, char vpk_file)>(); /*48 89 5C 24 ?? 48 89 6C 24 ?? 44 88 44 24 ?? 56 57 41 54 41 56 41 57 48 83 EC 20*/

inline CMemory p_CEngineAPI_ModInit = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x81\xEC\x00\x00\x00\x00\x4D\x8B\xF0"), "xxxx?xxxx?xxxx?xxxxxxxxxxxx????xxx");
inline auto CEngineAPI_ModInit = p_CEngineAPI_ModInit.RCast<bool (*)(CEngineAPI* pEngineAPI, const char* pModName, const char* pGameDir)>(); /*48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 4D 8B F0*/

inline CMemory p_CEngineAPI_MainLoop = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x55\x48\x81\xEC\x00\x00\x00\x00\x45\x33\xC9"), "xxxx?xxxx????xxx");
inline auto CEngineAPI_MainLoop = p_CEngineAPI_MainLoop.RCast<bool(*)(void)>(); /*48 89 5C 24 ? 55 48 81 EC ? ? ? ? 45 33 C9*/
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
inline CMemory p_PakFile_Init = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x44\x88\x44\x24\x00\x53\x55\x56\x57"), "xxxx?xxxx");
inline auto PakFile_Init = p_PakFile_Init.RCast<void (*)(char* buffer, char* source, char vpk_file)>(); /*44 88 44 24 ?? 53 55 56 57*/

inline CMemory p_CEngineAPI_ModInit = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x4C\x24\x00\x55\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x81\xEC\x00\x00\x00\x00\x4D\x8B\xF8"), "xxxx?xxxx?xxxxxxxxxxxxxx????xxx");
inline auto CEngineAPI_ModInit = p_CEngineAPI_ModInit.RCast<bool (*)(CEngineAPI* pEngineAPI, const char* pModName, const char* pGameDir)>(); /*48 89 5C 24 ? 48 89 4C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 4D 8B F8*/

inline CMemory p_CEngineAPI_MainLoop = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\xE8\x00\x00\x00\x00\x48\x8B\x15\x00\x00\x00\x00\x84\xC0\xB9\x00\x00\x00\x00"), "x????xxx????xxx????").FollowNearCallSelf();
inline auto CEngineAPI_MainLoop = p_CEngineAPI_MainLoop.RCast<bool(*)(void)>(); /*E8 ? ? ? ? 48 8B 15 ? ? ? ? 84 C0 B9 ? ? ? ?*/
#endif

#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
inline CMemory g_pMapVPKCache = p_PakFile_Init.FindPatternSelf("4C 8D 35 ?? ?? ?? ?? 44", CMemory::Direction::DOWN, 250).OffsetSelf(0x3).ResolveRelativeAddressSelf().GetPtr();
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
inline CMemory g_pMapVPKCache = p_PakFile_Init.FindPatternSelf("48 8D 1D ?? ?? ?? ?? 4C", CMemory::Direction::DOWN, 250).OffsetSelf(0x3).ResolveRelativeAddressSelf().GetPtr();
#endif

void SysDll2_Attach();
void SysDll2_Detach();
///////////////////////////////////////////////////////////////////////////////
class HSys_Dll2 : public IDetour
{
	virtual void GetAdr(void) const
	{
		std::cout << "| FUN: CEngineAPI::Connect                  : 0x" << std::hex << std::uppercase << p_CEngineAPI_Connect.GetPtr()  << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: CEngineAPI::ModInit                  : 0x" << std::hex << std::uppercase << p_CEngineAPI_ModInit.GetPtr()  << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: CEngineAPI::MainLoop                 : 0x" << std::hex << std::uppercase << p_CEngineAPI_MainLoop.GetPtr() << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: PakFile_Init                         : 0x" << std::hex << std::uppercase << p_PakFile_Init.GetPtr()        << std::setw(nPad) << " |" << std::endl;
		std::cout << "| VAR: g_pMapVPKCache                       : 0x" << std::hex << std::uppercase << g_pMapVPKCache.GetPtr()        << std::setw(nPad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HSys_Dll2);