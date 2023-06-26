#pragma once
#include "tier0/tslist.h"

#define PLATFORM_PAK_PATH "paks\\Win64\\"
#define PLATFORM_PAK_OVERRIDE_PATH "paks\\Win32\\"

#define INVALID_PAK_HANDLE -1

typedef int RPakHandle_t;
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
inline auto CPakFile_LoadAsync = p_CPakFile_LoadAsync.RCast<RPakHandle_t(*)(const char* szPakFileName, CAlignedMemAlloc* pMalloc, int nIdx, bool bUnk)>();

inline CMemory p_CPakFile_LoadPak;
inline auto CPakFile_LoadPak = p_CPakFile_LoadPak.RCast<unsigned int (*)(void* thisptr, void* a2, uint64_t a3)>();

inline CMemory p_CPakFile_UnloadPak;
inline auto CPakFile_UnloadPak = p_CPakFile_UnloadPak.RCast<void (*)(RPakHandle_t handle)>();

inline CMemory p_CPakFile_OpenFileOffset; // Offset to inlined 'CPakFile::LoadPak_OpenFile'.

class CPakFile
{
public:
	static RPakHandle_t LoadAsync(const char* szPakFileName, CAlignedMemAlloc* pMalloc = AlignedMemAlloc(), int nIdx = NULL, bool bUnk = false);
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
		LogFunAdr("CPakFile::OpenFile", p_CPakFile_OpenFileOffset.GetPtr());
	}
	virtual void GetFun(void) const
	{
		p_CPakFile_LoadPak = g_GameDll.FindPatternSIMD("48 89 4C 24 ?? 56 41 55 48 81 EC ?? ?? ?? ?? 4C");
		CPakFile_LoadPak = p_CPakFile_LoadPak.RCast<unsigned int (*)(void*, void*, uint64_t)>();

		p_CPakFile_LoadAsync = g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 89 03 8B 0B").FollowNearCallSelf();
		CPakFile_LoadAsync = p_CPakFile_LoadAsync.RCast<RPakHandle_t(*)(const char*, CAlignedMemAlloc*, int, bool)>();

		p_CPakFile_UnloadPak = g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 85 FF 74 0C").FollowNearCallSelf();
		CPakFile_UnloadPak = p_CPakFile_UnloadPak.RCast<void (*)(RPakHandle_t)>();
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const
	{
		p_CPakFile_OpenFileOffset = g_GameDll.FindPatternSIMD("48 89 7C 24 30 C7 44 24 28 ?? ?? ?? 40");
	}
	virtual void Attach(void) const;
	virtual void Detach(void) const;
};
///////////////////////////////////////////////////////////////////////////////
