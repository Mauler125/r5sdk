#pragma once
#include "tier0/basetypes.h"
#include "tier0/interface.h"

class CEngineAPI
{
	// TODO [ AMOS ]:
};

namespace
{
	ADDRESS p_CEngineAPI_Connect = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x83\xEC\x28\x48\x8B\x05\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00\x48\x85\xC0\x48\x89\x15", "xxxxxxx????xxx????xxxxxx");
	bool (*CEngineAPI_Connect)(CEngineAPI* thisptr, CreateInterfaceFn factory) = (bool (*)(CEngineAPI*, CreateInterfaceFn))p_CEngineAPI_Connect.GetPtr(); /*48 83 EC 28 48 8B 05 ? ? ? ? 48 8D 0D ? ? ? ? 48 85 C0 48 89 15 ? ? ? ?*/
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	ADDRESS p_PakFile_Init = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x44\x88\x44\x24\x00\x56\x57\x41\x54\x41\x56\x41\x57\x48\x83\xEC\x20", "xxxx?xxxx?xxxx?xxxxxxxxxxxx");
	int (*PakFile_Init)(char* buffer, char* source, char vpk_file) = (int (*)(char*, char*, char))p_PakFile_Init.GetPtr(); /*48 89 5C 24 ?? 48 89 6C 24 ?? 44 88 44 24 ?? 56 57 41 54 41 56 41 57 48 83 EC 20*/

	ADDRESS g_pMapVPKCache = p_PakFile_Init.FindPatternSelf("4C 8D 35 ?? ?? ?? ?? 44", ADDRESS::Direction::DOWN, 250).OffsetSelf(0x3).ResolveRelativeAddressSelf().GetPtr();
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	ADDRESS p_PakFile_Init = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x44\x88\x44\x24\x00\x53\x55\x56\x57", "xxxx?xxxx");
	void (*PakFile_Init)(char* buffer, char* source, char vpk_file) = (void (*)(char*, char*, char))p_PakFile_Init.GetPtr(); /*44 88 44 24 ?? 53 55 56 57*/

	ADDRESS g_pMapVPKCache = p_PakFile_Init.FindPatternSelf("48 8D 1D ?? ?? ?? ?? 4C", ADDRESS::Direction::DOWN, 250).OffsetSelf(0x3).ResolveRelativeAddressSelf().GetPtr();
#endif
}

///////////////////////////////////////////////////////////////////////////////
class HSys_Dll2 : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: CEngineAPI::Connect                  : 0x" << std::hex << std::uppercase << p_CEngineAPI_Connect.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: PakFile_Init                         : 0x" << std::hex << std::uppercase << p_PakFile_Init.GetPtr()       << std::setw(npad) << " |" << std::endl;
		std::cout << "| VAR: g_pMapVPKCache                       : 0x" << std::hex << std::uppercase << g_pMapVPKCache.GetPtr()       << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HSys_Dll2);