#pragma once

enum class ePakStatus : int
{
	PAK_STATUS_FREED = 0,
	PAK_STATUS_LOAD_PENDING,
	PAK_STATUS_REPAK_RUNNING,
	PAK_STATUS_REPAK_DONE,
	PAK_STATUS_LOAD_STARTING,
	PAK_STATUS_LOAD_PAKHDR,
	PAK_STATUS_LOAD_PATCH_INIT,
	PAK_STATUS_LOAD_PATCH_EDIT_STREAM,
	PAK_STATUS_LOAD_ASSETS,
	PAK_STATUS_LOADED, // 9
	PAK_STATUS_UNLOAD_PENDING,
	PAK_STATUS_FREE_PENDING,
	PAK_STATUS_CANCELING,
	PAK_STATUS_ERROR, // 13
	PAK_STATUS_INVALID_PAKHANDLE,
	PAK_STATUS_BUSY
};

namespace
{
	/* ==== RTECH_GAME ====================================================================================================================================================== */
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	ADDRESS p_RTech_UnloadAsset = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x83\xEC\x28\x48\x85\xC9\x0F\x84\x00\x00\x00\x00\x48\x8B\x05\x00\x00\x00\x00", "xxxxxxxxx????xxx????");
	void (*RTech_UnloadAsset)(std::int64_t a1) = (void (*)(std::int64_t))p_RTech_UnloadAsset.GetPtr(); /*48 83 EC 28 48 85 C9 0F 84 ? ? ? ? 48 8B 05 ? ? ? ? */

	ADDRESS p_JT_HelpWithAnything = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x41\x56\x48\x83\xEC\x30\x80\x3D\x00\x00\x00\x00\x00", "xxxx?xxxx?xxxx?xxxx?xxxxxxxx?????");
	void* (*JT_HelpWithAnything)(bool bShouldLoadPak) = (void* (*)(bool))p_JT_HelpWithAnything.GetPtr(); /*48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 56 48 83 EC 30 80 3D ? ? ? ? ?*/
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	ADDRESS p_RTech_UnloadAsset = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x83\xEC\x28\x48\x85\xD2\x74\x40\x48\x8B\x05\x00\x00\x00\x00", "xxxxxxxxxxxx????");
	void (*RTech_UnloadAsset)(std::int64_t a1, std::int64_t a2) = (void (*)(std::int64_t, std::int64_t))p_RTech_UnloadAsset.GetPtr(); /*48 83 EC 28 48 85 D2 74 40 48 8B 05 ? ? ? ?*/

	ADDRESS p_RTech_LoadPak = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x4C\x24\x00\x56\x41\x55", "xxxx?xxx"); /*48 89 4C 24 ? 56 41 55*/
	unsigned int (*RTech_LoadPak)(void* thisptr, void* a2, std::uint64_t a3) = (unsigned int (*)(void*, void*, std::uint64_t))p_RTech_LoadPak.GetPtr();

	ADDRESS p_RTech_LoadMapPak = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x81\xEC\x00\x00\x00\x00\x0F\xB6\x05\x00\x00\x00\x00\x4C\x8D\x05\x00\x00\x00\x00\x84\xC0", "xxx????xxx????xxx????xx");
	bool (*RTech_LoadMapPak)(const char* szPakFile) = (bool (*)(const char*))p_RTech_LoadMapPak.GetPtr(); /*48 81 EC ? ? ? ? 0F B6 05 ? ? ? ? 4C 8D 05 ? ? ? ? 84 C0*/

	ADDRESS p_JT_HelpWithAnything = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x30\x80\x3D\x00\x00\x00\x00\x00", "xxxx?xxxx?xxxxxxx?????");
	void* (*JT_HelpWithAnything)(bool bShouldLoadPak) = (void* (*)(bool))p_JT_HelpWithAnything.GetPtr(); /*48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 30 80 3D ? ? ? ? ?*/
#endif
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1) || defined (GAMEDLL_S2)
	ADDRESS p_RTech_AsyncLoad = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x40\x53\x48\x83\xEC\x40\x48\x89\x6C\x24\x00\x41\x8B\xE8", "xxxxxxxxxx?xxx"); /*40 53 48 83 EC 40 48 89 6C 24 ? 41 8B E8*/
	int (*RTech_AsyncLoad)(void* Src, std::uintptr_t pMalloc, int nIdx, char szPakFile) = (int (*)(void*, std::uintptr_t, int, char))p_RTech_AsyncLoad.GetPtr();
#elif defined (GAMEDLL_S3)
	ADDRESS p_RTech_AsyncLoad = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x40\x53\x48\x83\xEC\x40\x48\x89\x6C\x24\x00\x41\x0F\xB6\xE9", "xxxxxxxxxx?xxxx"); /*40 53 48 83 EC 40 48 89 6C 24 ? 41 0F B6 E9*/
	int (*RTech_AsyncLoad)(void* Src, std::uintptr_t pMalloc, int nIdx, char szPakFile) = (int (*)(void*, std::uintptr_t, int, char))p_RTech_AsyncLoad.GetPtr();
#endif
	ADDRESS p_RTech_UnloadPak = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x30\x8B\xC1", "xxxx?xxxx?xxxxxxx");
	void* (*RTech_UnloadPak)(int nPakId) = (void* (*)(int nPakId))p_RTech_UnloadPak.GetPtr();/*48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 30 8B C1*/
}
void HRTech_UnloadAsset(std::int64_t a1, std::int64_t a2);
void HRTech_AsyncLoad(std::string svPakFileName);

void RTech_Game_Attach();
void RTech_Game_Detach();

extern std::vector<int> g_nLoadedPakFileId;
///////////////////////////////////////////////////////////////////////////////
class HRTechGame : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: RTech_UnloadAsset                    : 0x" << std::hex << std::uppercase << p_RTech_UnloadAsset.GetPtr()   << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: RTech_AsyncLoad                      : 0x" << std::hex << std::uppercase << p_RTech_AsyncLoad.GetPtr()     << std::setw(npad) << " |" << std::endl;
#if defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		std::cout << "| FUN: RTech_LoadPak                        : 0x" << std::hex << std::uppercase << p_RTech_LoadPak.GetPtr()       << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: RTech_LoadMapPak                     : 0x" << std::hex << std::uppercase << p_RTech_LoadMapPak.GetPtr()    << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: RTech_UnloadPak                      : 0x" << std::hex << std::uppercase << p_RTech_UnloadPak.GetPtr()     << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: JT_HelpWithAnything                  : 0x" << std::hex << std::uppercase << p_JT_HelpWithAnything.GetPtr() << std::setw(npad) << " |" << std::endl;
#endif // GAMEDLL_S2 || GAMEDLL_S3
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HRTechGame);
