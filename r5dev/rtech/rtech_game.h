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
inline auto CPakFile_LoadAsync = p_CPakFile_LoadAsync.RCast<RPakHandle_t(*)(const char* szPakFileName, void* pMalloc, int nIdx, bool bUnk)>();

inline CMemory p_CPakFile_LoadPak;
inline auto CPakFile_LoadPak = p_CPakFile_LoadPak.RCast<unsigned int (*)(void* thisptr, void* a2, uint64_t a3)>();

inline CMemory p_CPakFile_UnloadPak;
inline auto CPakFile_UnloadPak = p_CPakFile_UnloadPak.RCast<void (*)(RPakHandle_t handle)>();

inline CMemory p_CPakFile_LoadPak_OpenFileOffset;

class CPakFile
{
public:
	static RPakHandle_t LoadAsync(const char* szPakFileName, void* pMalloc = g_pMallocPool, int nIdx = NULL, bool bUnk = false);
	static void UnloadPak(RPakHandle_t handle);
};

extern CPakFile* g_pakLoadApi;
extern vector<RPakHandle_t> g_vLoadedPakHandle;

///////////////////////////////////////////////////////////////////////////////
class V_RTechGame : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CPakFile::LoadAsync", p_CPakFile_LoadAsync.GetPtr());
		LogFunAdr("CPakFile::LoadPak", p_CPakFile_LoadPak.GetPtr());
		LogFunAdr("CPakFile::UnloadPak", p_CPakFile_UnloadPak.GetPtr());
		LogConAdr("CPakFile::LoadPak_OpenFileOffset", p_CPakFile_LoadPak_OpenFileOffset.GetPtr());
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CPakFile_LoadPak = g_GameDll.FindPatternSIMD("48 89 4C 24 ?? 56 41 55 48 81 EC ?? ?? ?? ?? 4C 8B 69 60");
		CPakFile_LoadPak = p_CPakFile_LoadPak.RCast<unsigned int (*)(void*, void*, uint64_t)>(); /*48 89 4C 24 ? 56 41 55 48 81 EC ? ? ? ? 4C 8B 69 60*/
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_CPakFile_LoadPak = g_GameDll.FindPatternSIMD("48 89 4C 24 ?? 56 41 55"); /*48 89 4C 24 ? 56 41 55*/
		CPakFile_LoadPak = p_CPakFile_LoadPak.RCast<unsigned int (*)(void*, void*, uint64_t)>();
#endif
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1) || defined (GAMEDLL_S2)
		p_CPakFile_LoadAsync = g_GameDll.FindPatternSIMD("40 53 48 83 EC 40 48 89 6C 24 ?? 41 8B E8");
		CPakFile_LoadAsync = p_CPakFile_LoadAsync.RCast<RPakHandle_t(*)(const char*, uintptr_t, int, bool)>(); /*40 53 48 83 EC 40 48 89 6C 24 ? 41 8B E8*/
#elif defined (GAMEDLL_S3)
		p_CPakFile_LoadAsync = g_GameDll.FindPatternSIMD("40 53 48 83 EC 40 48 89 6C 24 ?? 41 0F B6 E9");
		CPakFile_LoadAsync = p_CPakFile_LoadAsync.RCast<RPakHandle_t(*)(const char*, void*, int, bool)>(); /*40 53 48 83 EC 40 48 89 6C 24 ? 41 0F B6 E9*/
#endif
		p_CPakFile_UnloadPak = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 30 8B C1");
		CPakFile_UnloadPak = p_CPakFile_UnloadPak.RCast<void (*)(RPakHandle_t)>(); /*48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 30 8B C1*/
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const
	{
		p_CPakFile_LoadPak_OpenFileOffset = g_GameDll.FindPatternSIMD("48 89 7C 24 30 C7 44 24 28 ?? ?? ?? 40"); /*48 89 7C 24 30 C7 44 24 28 00 00 00 40*/
	}
	virtual void Attach(void) const;
	virtual void Detach(void) const;
};
///////////////////////////////////////////////////////////////////////////////
