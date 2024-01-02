#pragma once
#include "tier0/tslist.h"
#include "launcher/launcher.h"
#include "public/rtech/ipakfile.h"

/* ==== RTECH_GAME ====================================================================================================================================================== */
inline PakHandle_t(*v_Pak_LoadAsync)(const char* fileName, CAlignedMemAlloc* allocator, int nIdx, bool bUnk);
inline EPakStatus(*v_Pak_WaitAsync)(PakHandle_t handle, void* finishCallback);
inline unsigned int (*v_Pak_LoadPak)(void* thisptr, void* a2, uint64_t a3);
inline void(*v_Pak_UnloadPak)(PakHandle_t handle);
inline int(*v_Pak_OpenFile)(const CHAR* fileName, int64_t unused, LONGLONG* outFileSize);
inline void(*v_Pak_CloseFile)(short fileHandle);

inline CMemory p_Pak_OpenFileOffset; // Offset to inlined 'Pak_OpenFile'.

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
		LogFunAdr("Pak_LoadAsync", v_Pak_LoadAsync);
		LogFunAdr("Pak_WaitAsync", v_Pak_WaitAsync);
		LogFunAdr("Pak_LoadPak", v_Pak_LoadPak);
		LogFunAdr("Pak_UnloadPak", v_Pak_UnloadPak);
		LogFunAdr("Pak_OpenFile", v_Pak_OpenFile);
		LogFunAdr("Pak_CloseFile", v_Pak_CloseFile);
		LogFunAdr("Pak_ProcessGuidRelationsForAsset", v_Pak_ProcessGuidRelationsForAsset);
		LogVarAdr("g_pakLoadApi", g_pakLoadApi);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 89 03 8B 0B").FollowNearCallSelf().GetPtr(v_Pak_LoadAsync);
		g_GameDll.FindPatternSIMD("40 53 55 48 83 EC 38 48 89 74 24 ??").GetPtr(v_Pak_WaitAsync);
		g_GameDll.FindPatternSIMD("48 89 4C 24 ?? 56 41 55 48 81 EC ?? ?? ?? ?? 4C").GetPtr(v_Pak_LoadPak);
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 85 FF 74 0C").FollowNearCallSelf().GetPtr(v_Pak_UnloadPak);
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 89 85 08 01 ?? ??").FollowNearCallSelf().GetPtr(v_Pak_OpenFile);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 8B D9 48 8D 35 ?? ?? ?? ??").GetPtr(v_Pak_CloseFile);
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 48 8B 86 ?? ?? ?? ?? 42 8B 0C B0").FollowNearCallSelf().GetPtr(v_Pak_ProcessGuidRelationsForAsset);
	}
	virtual void GetVar(void) const
	{
		g_pakLoadApi = CMemory(v_LauncherMain).Offset(0x820).FindPatternSelf("48 89").ResolveRelativeAddressSelf(0x3, 0x7).RCast<PakLoadFuncs_t*>();
	}
	virtual void GetCon(void) const
	{
		p_Pak_OpenFileOffset = g_GameDll.FindPatternSIMD("48 89 7C 24 30 C7 44 24 28 ?? ?? ?? 40");
	}
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
