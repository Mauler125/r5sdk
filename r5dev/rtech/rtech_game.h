#pragma once
#include "tier0/tslist.h"
#include "launcher/launcher.h"
#include "public/rtech/ipakfile.h"

/* ==== RTECH_GAME ====================================================================================================================================================== */
inline CMemory p_Pak_LoadAsync;
inline PakHandle_t(*v_Pak_LoadAsync)(const char* fileName, CAlignedMemAlloc* allocator, int nIdx, bool bUnk);

inline CMemory p_Pak_WaitAsync;
inline EPakStatus(*v_Pak_WaitAsync)(PakHandle_t handle, void* finishCallback);

inline CMemory p_Pak_LoadPak;
inline unsigned int (*v_Pak_LoadPak)(void* thisptr, void* a2, uint64_t a3);

inline CMemory p_Pak_UnloadPak;
inline void(*v_Pak_UnloadPak)(PakHandle_t handle);

inline CMemory p_Pak_OpenFile;
inline int(*v_Pak_OpenFile)(const CHAR* fileName, int64_t unused, LONGLONG* outFileSize);

inline CMemory p_Pak_CloseFile;
inline void(*v_Pak_CloseFile)(short fileHandle);

inline CMemory p_Pak_OpenFileOffset; // Offset to inlined 'Pak_OpenFile'.

inline CMemory p_Pak_ProcessGuidRelationsForAsset;
inline void(*v_Pak_ProcessGuidRelationsForAsset)(PakFile_t* pak, PakAsset_t* asset);

typedef struct PakLoadFuncs_s
{
	void* LoadPatches;
	void* RegisterAsset;
	char unknown0[8];
	PakHandle_t (*LoadAsync)(const char* pakFileName, CAlignedMemAlloc* allocator, int nIdx, bool bUnk);
	void* Func4;
	void (*UnloadPak)(PakHandle_t handle);
	void* Func6;
	char unknown2[16];
	void* Func7;
	void* Func8;
	EPakStatus(*WaitAsync)(PakHandle_t handle, void* finishCallback);
	void* Func10;
	void* Func11;
	void* FindByGUID;
	void* FindByName;
	char unknown3[8];
	void* Func14;
	void* Func15;
	void* Func16;
	void* Func17;
	void* Func18;
	void* IncrementStreamingAssetCount;
	void* DecrementStreamingAssetCount;
	void* IsFullStreamingInstall;
	char unknown4[48];
	void* OpenFile;
	void* CloseFile;
	void* Func24;
	void* Func25;
	void* Func26;
	void* QueueAsyncFileRead;
	void* Func28;
	void* Func29;
	void* WaitForAsyncFileRead;
	void* Func31;
	void* Func32;
	void* Func33;
} PakLoadFuncs_t;

extern PakLoadFuncs_t* g_pakLoadApi;
extern CUtlVector<PakHandle_t> g_vLoadedPakHandle;

///////////////////////////////////////////////////////////////////////////////
class V_RTechGame : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("Pak_LoadAsync", p_Pak_LoadAsync.GetPtr());
		LogFunAdr("Pak_WaitAsync", p_Pak_WaitAsync.GetPtr());
		LogFunAdr("Pak_LoadPak", p_Pak_LoadPak.GetPtr());
		LogFunAdr("Pak_UnloadPak", p_Pak_UnloadPak.GetPtr());
		LogFunAdr("Pak_OpenFile", p_Pak_OpenFile.GetPtr());
		LogFunAdr("Pak_CloseFile", p_Pak_CloseFile.GetPtr());
		LogFunAdr("Pak_ProcessGuidRelationsForAsset", p_Pak_ProcessGuidRelationsForAsset.GetPtr());
		LogVarAdr("g_pakLoadApi", reinterpret_cast<uintptr_t>(g_pakLoadApi));
	}
	virtual void GetFun(void) const
	{
		p_Pak_LoadAsync = g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 89 03 8B 0B").FollowNearCallSelf();
		v_Pak_LoadAsync = p_Pak_LoadAsync.RCast<PakHandle_t(*)(const char*, CAlignedMemAlloc*, int, bool)>();

		p_Pak_WaitAsync = g_GameDll.FindPatternSIMD("40 53 55 48 83 EC 38 48 89 74 24 ??");
		v_Pak_WaitAsync = p_Pak_WaitAsync.RCast<EPakStatus(*)(PakHandle_t, void*)>();

		p_Pak_LoadPak = g_GameDll.FindPatternSIMD("48 89 4C 24 ?? 56 41 55 48 81 EC ?? ?? ?? ?? 4C");
		v_Pak_LoadPak = p_Pak_LoadPak.RCast<unsigned int (*)(void*, void*, uint64_t)>();

		p_Pak_UnloadPak = g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 85 FF 74 0C").FollowNearCallSelf();
		v_Pak_UnloadPak = p_Pak_UnloadPak.RCast<void (*)(PakHandle_t)>();

		p_Pak_OpenFile = g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 89 85 08 01 ?? ??").FollowNearCallSelf();
		v_Pak_OpenFile = p_Pak_OpenFile.RCast<int (*)(const char*, int64_t, ssize_t*)>();

		p_Pak_CloseFile = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 8B D9 48 8D 35 ?? ?? ?? ??");
		v_Pak_CloseFile = p_Pak_CloseFile.RCast<void (*)(short)>();

#ifdef GAMEDLL_S3
		p_Pak_ProcessGuidRelationsForAsset = g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 48 8B 86 ?? ?? ?? ?? 42 8B 0C B0").FollowNearCallSelf();
		v_Pak_ProcessGuidRelationsForAsset = p_Pak_ProcessGuidRelationsForAsset.RCast<void(*)(PakFile_t*, PakAsset_t*)>();
#endif
	}
	virtual void GetVar(void) const
	{
		g_pakLoadApi = p_LauncherMain.Offset(0x820).FindPatternSelf("48 89").ResolveRelativeAddressSelf(0x3, 0x7).RCast<PakLoadFuncs_t*>();
	}
	virtual void GetCon(void) const
	{
		p_Pak_OpenFileOffset = g_GameDll.FindPatternSIMD("48 89 7C 24 30 C7 44 24 28 ?? ?? ?? 40");
	}
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
