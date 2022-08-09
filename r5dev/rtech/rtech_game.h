#pragma once
#include "tier0/tslist.h"

typedef int RPakHandle_t;
constexpr int INVALID_PAK_HANDLE = -1;

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

/* ==== RTECH_GAME ====================================================================================================================================================== */
inline CMemory p_CPakFile_LoadAsync;
inline auto CPakFile_LoadAsync = p_CPakFile_LoadAsync.RCast<RPakHandle_t(*)(const char* szPakFileName, uintptr_t pMalloc, int nIdx, bool bUnk)>();

inline CMemory p_CPakFile_LoadPak;
inline auto CPakFile_LoadPak = p_CPakFile_LoadPak.RCast<unsigned int (*)(void* thisptr, void* a2, uint64_t a3)>();

inline CMemory p_CPakFile_UnloadPak;
inline auto CPakFile_UnloadPak = p_CPakFile_UnloadPak.RCast<void (*)(RPakHandle_t handle)>();

class CPakFile
{
public:
	static RPakHandle_t LoadAsync(const char* szPakFileName, uintptr_t pMalloc = g_pMallocPool.GetPtr(), int nIdx = NULL, bool bUnk = false);
	static void UnloadPak(RPakHandle_t handle);
};
extern CPakFile* g_pakLoadApi;

void RTech_Game_Attach();
void RTech_Game_Detach();

extern vector<RPakHandle_t> g_vLoadedPakHandle;
///////////////////////////////////////////////////////////////////////////////
class VRTechGame : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: CPakFile::LoadAsync                  : {:#18x} |\n", p_CPakFile_LoadAsync.GetPtr());
		spdlog::debug("| FUN: CPakFile::LoadPak                    : {:#18x} |\n", p_CPakFile_LoadPak.GetPtr());
		spdlog::debug("| FUN: CPakFile::UnloadPak                  : {:#18x} |\n", p_CPakFile_UnloadPak.GetPtr());
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CPakFile_LoadPak = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x4C\x24\x00\x56\x41\x55\x48\x81\xEC\x00\x00\x00\x00\x4C\x8B\x69\x60"), "xxxx?xxxxxx????xxxx");
		CPakFile_LoadPak = p_CPakFile_LoadPak.RCast<unsigned int (*)(void*, void*, uint64_t)>(); /*48 89 4C 24 ? 56 41 55 48 81 EC ? ? ? ? 4C 8B 69 60*/
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_CPakFile_LoadPak = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x4C\x24\x00\x56\x41\x55"), "xxxx?xxx"); /*48 89 4C 24 ? 56 41 55*/
		CPakFile_LoadPak = p_CPakFile_LoadPak.RCast<unsigned int (*)(void*, void*, uint64_t)>();
#endif
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1) || defined (GAMEDLL_S2)
		p_CPakFile_LoadAsync = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x40\x48\x89\x6C\x24\x00\x41\x8B\xE8"), "xxxxxxxxxx?xxx");
		CPakFile_LoadAsync = p_CPakFile_LoadAsync.RCast<RPakHandle_t(*)(const char*, uintptr_t, int, bool)>(); /*40 53 48 83 EC 40 48 89 6C 24 ? 41 8B E8*/
#elif defined (GAMEDLL_S3)
		p_CPakFile_LoadAsync = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x40\x48\x89\x6C\x24\x00\x41\x0F\xB6\xE9"), "xxxxxxxxxx?xxxx");
		CPakFile_LoadAsync = p_CPakFile_LoadAsync.RCast<RPakHandle_t(*)(const char*, uintptr_t, int, bool)>(); /*40 53 48 83 EC 40 48 89 6C 24 ? 41 0F B6 E9*/
#endif
		p_CPakFile_UnloadPak = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x30\x8B\xC1"), "xxxx?xxxx?xxxxxxx");
		CPakFile_UnloadPak = p_CPakFile_UnloadPak.RCast<void (*)(RPakHandle_t)>(); /*48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 30 8B C1*/
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VRTechGame);
