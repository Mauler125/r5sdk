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

namespace
{
	/* ==== HOST ============================================================================================================================================================ */
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	ADDRESS p_Host_Init = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\xD9\xFF\x15\x00\x00\x00\x00", "xxxx?xxxx?xxxx?xxxxxxxxxxxx????xxxxx????");
	void* (*Host_Init)(bool* bDedicated) = (void* (*)(bool*))p_Host_Init.GetPtr(); /*48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 48 8B D9 FF 15 ? ? ? ?*/

	ADDRESS p_Host_NewGame = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x8B\xC4\x56\x41\x54\x41\x57\x48\x81\xEC\x00\x00\x00\x00\xF2\x0F\x10\x05\x00\x00\x00\x00", "xxxxxxxxxxx????xxxx????"); /*48 8B C4 56 41 54 41 57 48 81 EC ? ? ? ? F2 0F 10 05 ? ? ? ?*/
	bool (*Host_NewGame)(char* pszMapName, char* pszMapGroup, bool bLoadGame, char bBackground, LARGE_INTEGER PerformanceCount) = (bool (*)(char*, char*, bool, char, LARGE_INTEGER))p_Host_NewGame.GetPtr();
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	ADDRESS p_Host_Init = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\xAC\x24\x00\x00\x00\x00\xB8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x2B\xE0\x48\x8B\xD9", "xxxx?xxxx?xxxx?xxxxxxxxxxxxx????x????x????xxxxxx");
	void* (*Host_Init)(bool* bDedicated) = (void* (*)(bool*))p_Host_Init.GetPtr(); /*48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 55 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? B8 ? ? ? ? E8 ? ? ? ? 48 2B E0 48 8B D9*/

	ADDRESS p_Host_NewGame = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x8B\xC4\x00\x41\x54\x41\x55\x48\x81\xEC\x70\x04\x00\x00\xF2\x0F\x10\x05\x00\x00\x00\x0B", "xxx?xxxxxxxxxxxxxxx???x"); /*48 8B C4 ?? 41 54 41 55 48 81 EC 70 04 00 00 F2 0F 10 05 ?? ?? ?? 0B*/
	bool (*Host_NewGame)(char* pszMapName, char* pszMapGroup, bool bLoadGame, char bBackground, LARGE_INTEGER PerformanceCount) = (bool (*)(char*, char*, bool, char, LARGE_INTEGER))p_Host_NewGame.GetPtr();
#endif
	ADDRESS p_malloc_internal = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\xE9\x00\x00\x00\x00\xCC\xCC\xCC\x40\x53\x48\x83\xEC\x20\x48\x8D\x05\x00\x00\x00\x00", "x????xxxxxxxxxxxx????");
	void* (*malloc_internal)(void* pPool, int64_t size) = (void* (*)(void*, int64_t))p_malloc_internal.GetPtr(); /*E9 ? ? ? ? CC CC CC 40 53 48 83 EC 20 48 8D 05 ? ? ? ?*/
}

namespace
{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	ADDRESS g_pMallocPool = p_Host_Init.Offset(0x600).FindPatternSelf("48 8D 15 ?? ?? ?? 01", ADDRESS::Direction::DOWN, 100).ResolveRelativeAddressSelf(0x3, 0x7);
	static ADDRESS g_pEngineParmsBuffer = p_CModAppSystemGroup_Main.Offset(0x0).FindPatternSelf("48 8B", ADDRESS::Direction::DOWN, 100).ResolveRelativeAddress(0x3, 0x7);
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	ADDRESS g_pMallocPool = p_Host_Init.Offset(0x130).FindPatternSelf("48 8D 15 ?? ?? ?? 01", ADDRESS::Direction::DOWN, 100).ResolveRelativeAddressSelf(0x3, 0x7);
	static ADDRESS g_pEngineParmsBuffer = p_CModAppSystemGroup_Main.Offset(0x0).FindPatternSelf("4C 8B", ADDRESS::Direction::DOWN, 100).ResolveRelativeAddress(0x3, 0x7);
#endif
}

///////////////////////////////////////////////////////////////////////////////
class HHostCmd : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: Host_Init                            : 0x" << std::hex << std::uppercase << p_Host_Init.GetPtr()          << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: Host_NewGame                         : 0x" << std::hex << std::uppercase << p_Host_NewGame.GetPtr()       << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: malloc_internal                      : 0x" << std::hex << std::uppercase << p_malloc_internal.GetPtr()    << std::setw(npad) << " |" << std::endl;
		std::cout << "| VAR: g_pEngineParms                       : 0x" << std::hex << std::uppercase << g_pEngineParmsBuffer.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "| VAR: g_pMallocPool                        : 0x" << std::hex << std::uppercase << g_pMallocPool.GetPtr()        << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HHostCmd);
