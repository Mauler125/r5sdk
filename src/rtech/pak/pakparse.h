#ifndef RTECH_PAKPARSE_H
#define RTECH_PAKPARSE_H
#include "tier0/tslist.h"
#include "tier0/jobthread.h"
#include "launcher/launcher.h"

#include "rtech/ipakfile.h"
#include "rtech/async/asyncio.h"

// This function returns the pak handle of the patch master RPak
inline PakHandle_t(*v_Pak_Initialize)(int mode);

inline PakHandle_t(*v_Pak_LoadAsync)(const char* fileName, CAlignedMemAlloc* allocator, int nIdx, bool bUnk);
inline EPakStatus(*v_Pak_WaitAsync)(PakHandle_t handle, void* finishCallback);
inline void(*v_Pak_UnloadAsync)(PakHandle_t handle);
inline void(*v_Pak_RegisterAsset)(int, int, const char*, void*, void*, void*, void*, int, int, uint32_t, int, int);

inline bool(*v_Pak_StartLoadingPak)(PakLoadedInfo_t* loadedInfo);
inline bool(*v_Pak_ProcessPakFile)(PakFile_t* const pak);
inline bool(*v_Pak_ProcessAssets)(PakLoadedInfo_t* pakInfo);
inline void(*v_Pak_ResolveAssetRelations)(PakFile_t* const pak, const PakAsset_t* const asset);

inline void (*v_Pak_RunAssetLoadingJobs)(PakFile_t* pak);
inline void (*Pak_ProcessAssetRelationsAndResolveDependencies)(PakFile_t* pak_arg, PakAsset_t* asset_arg, unsigned int asset_idx_arg, unsigned int a4);

inline int  (*Pak_TrackAsset)(PakFile_t* const a1, PakAsset_t* a2);

// TODO: name these!
inline void (*sub_14043D870)(PakLoadedInfo_t* a1, int a2);

typedef struct PakLoadFuncs_s
{
	void* Initialize; // Returns the pak handle of the patch master RPak once initialized.
	void* RegisterAsset;
	char unknown0[8];
	PakHandle_t(*LoadAsync)(const char* pakFileName, CAlignedMemAlloc* allocator, int nIdx, bool bUnk);
	void* LoadAsyncAndWait;
	void (*UnloadAsync)(PakHandle_t handle);
	void* UnloadAsyncAndWait;
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
	int (*OpenAsyncFile)(const char* const fileName, int logLevel, size_t* const outFileSize);
	void (*CloseAsyncFile)(short fileHandle);
	void* Func24;
	void* Func25;
	void* ReadAsyncFile;
	void* ReadAsyncFileWithUserData;
	uint8_t (*CheckAsyncRequest)(int idx, size_t* const bytesProcessed, const char** const statusMsg);
	uint8_t (*WaitAndCheckAsyncRequest)(int idx, size_t* const bytesProcessed, const char** const statusMsg);
	void* WaitForAsyncFileRead;
	void* Func31;
	void* Func32;
	void* Func33;
} PakLoadFuncs_t;

extern PakLoadFuncs_t* g_pakLoadApi;

///////////////////////////////////////////////////////////////////////////////
class V_PakParse : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("Pak_Initialize", v_Pak_Initialize);

		LogFunAdr("Pak_LoadAsync", v_Pak_LoadAsync);
		LogFunAdr("Pak_WaitAsync", v_Pak_WaitAsync);
		LogFunAdr("Pak_UnloadAsync", v_Pak_UnloadAsync);

		LogFunAdr("Pak_RegisterAsset", v_Pak_RegisterAsset);

		LogFunAdr("Pak_StartLoadingPak", v_Pak_StartLoadingPak);

		LogFunAdr("Pak_ProcessPakFile", v_Pak_ProcessPakFile);
		LogFunAdr("Pak_ProcessAssets", v_Pak_ProcessAssets);
		LogFunAdr("Pak_ResolveAssetRelations", v_Pak_ResolveAssetRelations);

		LogVarAdr("g_pakLoadApi", g_pakLoadApi);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 8B D9 E8 ?? ?? ?? ??").GetPtr(v_Pak_Initialize);
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 89 03 8B 0B").FollowNearCallSelf().GetPtr(v_Pak_LoadAsync);
		g_GameDll.FindPatternSIMD("40 53 55 48 83 EC 38 48 89 74 24 ??").GetPtr(v_Pak_WaitAsync);
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 85 FF 74 0C").FollowNearCallSelf().GetPtr(v_Pak_UnloadAsync);
		g_GameDll.FindPatternSIMD("48 89 6C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 41 56 48 8B 44 24 ?? 4C 8D 35 ?? ?? ?? ??").GetPtr(v_Pak_RegisterAsset);

		g_GameDll.FindPatternSIMD("48 89 4C 24 ?? 56 41 55").GetPtr(v_Pak_StartLoadingPak);
		g_GameDll.FindPatternSIMD("40 53 55 56 41 54 41 56 48 81 EC ?? ?? ?? ??").GetPtr(v_Pak_ProcessPakFile);
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 84 C0 0F 84 ?? ?? ?? ?? 48 8B CF E8 ?? ?? ?? ?? 44 0F B7 05 ?? ?? ?? ??").FollowNearCallSelf().GetPtr(v_Pak_ProcessAssets);
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 48 8B 86 ?? ?? ?? ?? 42 8B 0C B0").FollowNearCallSelf().GetPtr(v_Pak_ResolveAssetRelations);

		g_GameDll.FindPatternSIMD("40 53 56 48 83 EC 58 44 8B 09").GetPtr(v_Pak_RunAssetLoadingJobs);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 41 54 41 55 41 56 41 57 48 83 EC 20 44 8B 0D ?? ?? ?? ?? 4C 8B E9").GetPtr(Pak_TrackAsset);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC 30 41 8B E9").GetPtr(Pak_ProcessAssetRelationsAndResolveDependencies);

		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? EB 14 48 8D 0D ?? ?? ?? ??").FollowNearCallSelf().GetPtr(sub_14043D870);
	}
	virtual void GetVar(void) const
	{
		g_pakLoadApi = CMemory(v_LauncherMain).Offset(0x820).FindPatternSelf("48 89").ResolveRelativeAddressSelf(0x3, 0x7).RCast<PakLoadFuncs_t*>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////

#endif // RTECH_PAKPARSE_H
