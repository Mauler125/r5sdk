#pragma once
#include "tier0/interface.h"

class CEngineAPI
{
public:
	// TODO [ AMOS ]:
};

namespace
{
	ADDRESS p_CEngineAPI_Connect = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x83\xEC\x28\x48\x8B\x05\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00\x48\x85\xC0\x48\x89\x15", "xxxxxxx????xxx????xxxxxx");
	bool (*CEngineAPI_Connect)(CEngineAPI* thisptr, CreateInterfaceFn factory) = (bool (*)(CEngineAPI*, CreateInterfaceFn))p_CEngineAPI_Connect.GetPtr(); /*48 83 EC 28 48 8B 05 ? ? ? ? 48 8D 0D ? ? ? ? 48 85 C0 48 89 15 ? ? ? ?*/

#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	ADDRESS p_PakFile_Init = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x44\x88\x44\x24\x00\x56\x57\x41\x54\x41\x56\x41\x57\x48\x83\xEC\x20", "xxxx?xxxx?xxxx?xxxxxxxxxxxx");
	int (*PakFile_Init)(char* buffer, char* source, char vpk_file) = (int (*)(char*, char*, char))p_PakFile_Init.GetPtr(); /*48 89 5C 24 ?? 48 89 6C 24 ?? 44 88 44 24 ?? 56 57 41 54 41 56 41 57 48 83 EC 20*/

	ADDRESS p_CEngineAPI_MainLoop = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x5C\x24\x00\x55\x48\x81\xEC\x00\x00\x00\x00\x45\x33\xC9", "xxxx?xxxx????xxx");
	void* (*CEngineAPI_MainLoop)() = (void* (*)())p_CEngineAPI_MainLoop.GetPtr(); /*48 89 5C 24 ? 55 48 81 EC ? ? ? ? 45 33 C9*/
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	ADDRESS p_PakFile_Init = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x44\x88\x44\x24\x00\x53\x55\x56\x57", "xxxx?xxxx");
	void (*PakFile_Init)(char* buffer, char* source, char vpk_file) = (void (*)(char*, char*, char))p_PakFile_Init.GetPtr(); /*44 88 44 24 ?? 53 55 56 57*/

	ADDRESS p_CEngineAPI_MainLoop = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\xE8\x00\x00\x00\x00\x48\x8B\x15\x00\x00\x00\x00\x84\xC0\xB9\x00\x00\x00\x00", "x????xxx????xxx????").FollowNearCallSelf();
	bool (*CEngineAPI_MainLoop)() = (bool(*)())p_CEngineAPI_MainLoop.GetPtr(); /*E8 ? ? ? ? 48 8B 15 ? ? ? ? 84 C0 B9 ? ? ? ?*/
#endif
}

namespace
{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	ADDRESS g_pMapVPKCache = p_PakFile_Init.FindPatternSelf("4C 8D 35 ?? ?? ?? ?? 44", ADDRESS::Direction::DOWN, 250).OffsetSelf(0x3).ResolveRelativeAddressSelf().GetPtr();
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	ADDRESS g_pMapVPKCache = p_PakFile_Init.FindPatternSelf("48 8D 1D ?? ?? ?? ?? 4C", ADDRESS::Direction::DOWN, 250).OffsetSelf(0x3).ResolveRelativeAddressSelf().GetPtr();
#endif
}

///////////////////////////////////////////////////////////////////////////////
class HSys_Dll2 : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: CEngineAPI::Connect                  : 0x" << std::hex << std::uppercase << p_CEngineAPI_Connect.GetPtr()  << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: CEngineAPI::MainLoop                 : 0x" << std::hex << std::uppercase << p_CEngineAPI_MainLoop.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: PakFile_Init                         : 0x" << std::hex << std::uppercase << p_PakFile_Init.GetPtr()        << std::setw(npad) << " |" << std::endl;
		std::cout << "| VAR: g_pMapVPKCache                       : 0x" << std::hex << std::uppercase << g_pMapVPKCache.GetPtr()        << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HSys_Dll2);