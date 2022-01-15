#pragma once

namespace
{
	/* ==== RTECH_GAME ====================================================================================================================================================== */
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	ADDRESS p_RTech_UnloadAsset = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x83\xEC\x28\x48\x85\xC9\x0F\x84\x00\x00\x00\x00\x48\x8B\x05\x00\x00\x00\x00", "xxxxxxxxx????xxx????");
	void (*RTech_UnloadAsset)(std::int64_t a1) = (void (*)(std::int64_t))p_RTech_UnloadAsset.GetPtr(); /*48 83 EC 28 48 85 C9 0F 84 ? ? ? ? 48 8B 05 ? ? ? ? */

#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	ADDRESS p_RTech_UnloadAsset = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x83\xEC\x28\x48\x85\xD2\x74\x40\x48\x8B\x05\x00\x00\x00\x00", "xxxxxxxxxxxx????");
	void (*RTech_UnloadAsset)(std::int64_t a1, std::int64_t a2) = (void (*)(std::int64_t, std::int64_t))p_RTech_UnloadAsset.GetPtr(); /*48 83 EC 28 48 85 D2 74 40 48 8B 05 ? ? ? ?*/
#endif

#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1) || defined (GAMEDLL_S2)
	ADDRESS p_RTech_AsyncLoad = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x40\x53\x48\x83\xEC\x40\x48\x89\x6C\x24\x00\x41\x8B\xE8", "xxxxxxxxxx?xxx");
	unsigned int (*RTech_AsyncLoad)(void* Src, __int64 a2, int a3, char pakfile) = (unsigned int (*)(void*, __int64, int, char))p_RTech_AsyncLoad.GetPtr(); /*40 53 48 83 EC 40 48 89 6C 24 ? 41 8B E8*/
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	ADDRESS p_RTech_AsyncLoad = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x40\x53\x48\x83\xEC\x40\x48\x89\x6C\x24\x00\x41\x0F\xB6\xE9", "xxxxxxxxxx?xxxx");
	unsigned int (*RTech_AsyncLoad)(void* Src, __int64 a2, int a3, char pakfile) = (unsigned int (*)(void*, __int64, int, char))p_RTech_AsyncLoad.GetPtr(); /*40 53 48 83 EC 40 48 89 6C 24 ? 41 0F B6 E9*/
#endif
}
void HRTech_UnloadAsset(std::int64_t a1, std::int64_t a2);
void HRtech_AsyncLoad(std::string svPakFileName);

void RTech_Game_Attach();
void RTech_Game_Detach();

///////////////////////////////////////////////////////////////////////////////
class HRTechGame : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: RTech_UnloadAsset                    : 0x" << std::hex << std::uppercase << p_RTech_UnloadAsset.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: RTech_AsyncLoad                      : 0x" << std::hex << std::uppercase << p_RTech_AsyncLoad.GetPtr()   << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HRTechGame);
