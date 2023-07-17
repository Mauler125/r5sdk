#pragma once
#include "tier0/tslist.h"
#include "public/rtech/ipakfile.h"

/* ==== RTECH_GAME ====================================================================================================================================================== */
inline CMemory p_CPakFile_LoadAsync;
inline RPakHandle_t(*CPakFile_LoadAsync)(const char* szPakFileName, CAlignedMemAlloc* pMalloc, int nIdx, bool bUnk);

inline CMemory p_CPakFile_WaitAsync;
inline RPakStatus_t(*CPakFile_WaitAsync)(RPakHandle_t handle, void* pFinishCallback);

inline CMemory p_CPakFile_LoadPak;
inline unsigned int (*CPakFile_LoadPak)(void* thisptr, void* a2, uint64_t a3);

inline CMemory p_CPakFile_UnloadPak;
inline void(*CPakFile_UnloadPak)(RPakHandle_t handle);

inline CMemory p_CPakFile_OpenFileOffset; // Offset to inlined 'CPakFile::LoadPak_OpenFile'.

class CPakFile
{
public:
	static        RPakHandle_t LoadAsync(const char* szPakFileName, CAlignedMemAlloc* pMalloc = AlignedMemAlloc(), int nIdx = NULL, bool bUnk = false);
	static inline RPakStatus_t WaitAsync(RPakHandle_t handle, void* pFinishCallback = nullptr) { return CPakFile_WaitAsync(handle, pFinishCallback); }
	static void UnloadPak(RPakHandle_t handle);
};

extern CPakFile* g_pakLoadApi;
extern CUtlVector<RPakHandle_t> g_vLoadedPakHandle;

///////////////////////////////////////////////////////////////////////////////
class V_RTechGame : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CPakFile::LoadAsync", p_CPakFile_LoadAsync.GetPtr());
		LogFunAdr("CPakFile::WaitAsync", p_CPakFile_WaitAsync.GetPtr());
		LogFunAdr("CPakFile::LoadPak", p_CPakFile_LoadPak.GetPtr());
		LogFunAdr("CPakFile::UnloadPak", p_CPakFile_UnloadPak.GetPtr());
		LogFunAdr("CPakFile::OpenFile", p_CPakFile_OpenFileOffset.GetPtr());
	}
	virtual void GetFun(void) const
	{
		p_CPakFile_LoadAsync = g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 89 03 8B 0B").FollowNearCallSelf();
		CPakFile_LoadAsync = p_CPakFile_LoadAsync.RCast<RPakHandle_t(*)(const char*, CAlignedMemAlloc*, int, bool)>();

		p_CPakFile_WaitAsync = g_GameDll.FindPatternSIMD("40 53 55 48 83 EC 38 48 89 74 24 ??");
		CPakFile_WaitAsync = p_CPakFile_WaitAsync.RCast<RPakStatus_t(*)(RPakHandle_t, void*)>();

		p_CPakFile_LoadPak = g_GameDll.FindPatternSIMD("48 89 4C 24 ?? 56 41 55 48 81 EC ?? ?? ?? ?? 4C");
		CPakFile_LoadPak = p_CPakFile_LoadPak.RCast<unsigned int (*)(void*, void*, uint64_t)>();

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
