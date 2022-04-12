#pragma once
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
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
inline CMemory p_Host_Init = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\xD9\xFF\x15\x00\x00\x00\x00"), "xxxx?xxxx?xxxx?xxxxxxxxxxxx????xxxxx????");
inline auto Host_Init = p_Host_Init.RCast<void* (*)(bool* bDedicated)>(); /*48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 48 8B D9 FF 15 ? ? ? ?*/

inline CMemory p_Host_NewGame = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\xC4\x56\x41\x54\x41\x57\x48\x81\xEC\x00\x00\x00\x00\xF2\x0F\x10\x05\x00\x00\x00\x00"), "xxxxxxxxxxx????xxxx????"); /*48 8B C4 56 41 54 41 57 48 81 EC ? ? ? ? F2 0F 10 05 ? ? ? ?*/
inline bool (*Host_NewGame)(char* pszMapName, char* pszMapGroup, bool bLoadGame, char bBackground, LARGE_INTEGER PerformanceCount) = (bool (*)(char*, char*, bool, char, LARGE_INTEGER))p_Host_NewGame.GetPtr();

inline CMemory p_Host_ChangeLevel = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x56\x41\x56\x48\x81\xEC\x00\x00\x00\x00\x49\x8B\xD8"), "xxxxxxxx????xxx");
inline auto Host_ChangeLevel = p_Host_ChangeLevel.RCast<bool (*)(bool bLoadFromSavedGame, const char* pszMapName, const char* pszMapGroup)>(); /*40 53 56 41 56 48 81 EC ? ? ? ? 49 8B D8*/
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
inline CMemory p_Host_Init = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\xAC\x24\x00\x00\x00\x00\xB8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x2B\xE0\x48\x8B\xD9"), "xxxx?xxxx?xxxx?xxxxxxxxxxxxx????x????x????xxxxxx");
inline auto Host_Init = p_Host_Init.RCast<void* (*)(bool* bDedicated)>(); /*48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 55 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? B8 ? ? ? ? E8 ? ? ? ? 48 2B E0 48 8B D9*/

inline CMemory p_Host_NewGame = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\xC4\x00\x41\x54\x41\x55\x48\x81\xEC\x70\x04\x00\x00\xF2\x0F\x10\x05\x00\x00\x00\x0B"), "xxx?xxxxxxxxxxxxxxx???x");
inline auto Host_NewGame = p_Host_NewGame.RCast<bool (*)(char* pszMapName, char* pszMapGroup, bool bLoadGame, char bBackground, LARGE_INTEGER PerformanceCount)>(); /*48 8B C4 ?? 41 54 41 55 48 81 EC 70 04 00 00 F2 0F 10 05 ?? ?? ?? 0B*/

inline CMemory p_Host_ChangeLevel = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x56\x57\x41\x56\x48\x81\xEC\x00\x00\x00\x00"), "xxxxxxxx????");
inline auto Host_ChangeLevel = p_Host_ChangeLevel.RCast<bool (*)(bool bLoadFromSavedGame, const char* pszMapName, const char* pszMapGroup)>(); /*40 56 57 41 56 48 81 EC ? ? ? ?*/
#endif
inline CMemory p_malloc_internal = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\xE9\x00\x00\x00\x00\xCC\xCC\xCC\x40\x53\x48\x83\xEC\x20\x48\x8D\x05\x00\x00\x00\x00"), "x????xxxxxxxxxxxx????");
inline auto malloc_internal = p_malloc_internal.RCast<void* (*)(void* pBool, int64_t nSize)>(); /*E9 ? ? ? ? CC CC CC 40 53 48 83 EC 20 48 8D 05 ? ? ? ?*/

#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
inline CMemory g_pMallocPool = p_Host_Init.Offset(0x600).FindPatternSelf("48 8D 15 ?? ?? ?? 01", CMemory::Direction::DOWN, 100).ResolveRelativeAddressSelf(0x3, 0x7);
inline static CModule g_pEngineParmsBuffer = p_CModAppSystemGroup_Main.Offset(0x0).FindPatternSelf("48 8B", CMemory::Direction::DOWN, 100).ResolveRelativeAddress(0x3, 0x7);
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
inline CMemory g_pMallocPool = p_Host_Init.Offset(0x130).FindPatternSelf("48 8D 15 ?? ?? ?? 01", CMemory::Direction::DOWN, 100).ResolveRelativeAddressSelf(0x3, 0x7);
inline static CMemory g_pEngineParmsBuffer = p_CModAppSystemGroup_Main.Offset(0x0).FindPatternSelf("4C 8B", CMemory::Direction::DOWN, 100).ResolveRelativeAddress(0x3, 0x7);
#endif


///////////////////////////////////////////////////////////////////////////////
class HHostCmd : public IDetour
{
	virtual void GetAdr(void) const
	{
		std::cout << "| FUN: Host_Init                            : 0x" << std::hex << std::uppercase << p_Host_Init.GetPtr()          << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: Host_NewGame                         : 0x" << std::hex << std::uppercase << p_Host_NewGame.GetPtr()       << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: Host_ChangeLevel                     : 0x" << std::hex << std::uppercase << p_Host_ChangeLevel.GetPtr()   << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: malloc_internal                      : 0x" << std::hex << std::uppercase << p_malloc_internal.GetPtr()    << std::setw(nPad) << " |" << std::endl;
		std::cout << "| VAR: g_pEngineParms                       : 0x" << std::hex << std::uppercase << g_pEngineParmsBuffer.GetPtr() << std::setw(nPad) << " |" << std::endl;
		std::cout << "| VAR: g_pMallocPool                        : 0x" << std::hex << std::uppercase << g_pMallocPool.GetPtr()        << std::setw(nPad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HHostCmd);
