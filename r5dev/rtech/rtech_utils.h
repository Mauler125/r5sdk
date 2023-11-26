#pragma once
#include "tier0/jobthread.h"
#include "vpklib/packedstore.h"
#include "rtech/rtech_game.h"
#include "public/rendersystem/schema/texture.g.h"
#include "public/rtech/ipakfile.h"

/* ==== RTECH =========================================================================================================================================================== */
// [ PIXIE ]: I'm very unsure about this, but it really seems like it
inline CMemory p_RTech_FindFreeSlotInFiles;
inline int32_t(*RTech_FindFreeSlotInFiles)(int32_t*);

inline CMemory p_RTech_RegisterAsset;
inline void(*RTech_RegisterAsset)(int, int, const char*, void*, void*, void*, void*, int, int, uint32_t, int, int);

inline CMemory p_StreamDB_Init;
inline void(*v_StreamDB_Init)(const char* pszLevelName);

inline PakLoadedInfo_t* g_pLoadedPakInfo;
inline int16_t* g_pRequestedPakCount;
inline int16_t* g_pLoadedPakCount;
inline JobID_t* g_pPakLoadJobID;
inline PakGlobals_t* g_pPakGlobals;

inline int32_t* s_pFileArray;
inline PSRWLOCK* s_pFileArrayMutex;
inline pFileHandleTracker_t* s_pFileHandles;

inline JobFifoLock_s* g_pPakFifoLock;
inline void* g_pPakFifoLockWrapper; // Pointer to functor that takes the global pak fifolock as argument.
inline bool* g_bPakFifoLockAcquired;

class RTech
{
public:
	uint64_t __fastcall StringToGuid(const char* pData);
	uint8_t __fastcall DecompressPakFile(PakDecompState_t* state, uint64_t inLen, uint64_t outLen);
	uint64_t __fastcall DecompressPakFileInit(PakDecompState_t* state, uint8_t* fileBuffer, uint64_t fileSize, uint64_t offNoHeader, uint64_t headerSize);
	PakLoadedInfo_t* GetPakLoadedInfo(PakHandle_t nPakId);
	PakLoadedInfo_t* GetPakLoadedInfo(const char* szPakName);
	const char* PakStatusToString(EPakStatus status);

	void** LoadShaderSet(void** VTablePtr);
};

///////////////////////////////////////////////////////////////////////////////
extern RTech* g_pRTech;

///////////////////////////////////////////////////////////////////////////////
class V_RTechUtils : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("RTech::FindFreeSlotInFiles", p_RTech_FindFreeSlotInFiles.GetPtr());

		LogFunAdr("StreamDB_Init", p_StreamDB_Init.GetPtr());
		LogVarAdr("s_FileArray", reinterpret_cast<uintptr_t>(s_pFileArray));
		LogVarAdr("s_FileArrayMutex", reinterpret_cast<uintptr_t>(s_pFileArrayMutex));
		LogVarAdr("s_FileHandles", reinterpret_cast<uintptr_t>(s_pFileHandles));

		LogVarAdr("g_loadedPakInfo", reinterpret_cast<uintptr_t>(g_pLoadedPakInfo));
		LogVarAdr("g_loadedPakCount", reinterpret_cast<uintptr_t>(g_pLoadedPakCount));
		LogVarAdr("g_requestedPakCount", reinterpret_cast<uintptr_t>(g_pRequestedPakCount));

		LogVarAdr("g_pakGlobals", reinterpret_cast<uintptr_t>(g_pPakGlobals));
		LogVarAdr("g_pakLoadJobID", reinterpret_cast<uintptr_t>(g_pPakLoadJobID));

		LogVarAdr("g_pakFifoLock", reinterpret_cast<uintptr_t>(g_pPakFifoLock));
		LogVarAdr("g_pakFifoLockWrapper", reinterpret_cast<uintptr_t>(g_pPakFifoLockWrapper));
		LogVarAdr("g_pakFifoLockAcquired", reinterpret_cast<uintptr_t>(g_bPakFifoLockAcquired));
	}
	virtual void GetFun(void) const 
	{
		p_StreamDB_Init = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 41 54 41 56 41 57 48 83 EC 40 48 8B E9");
		v_StreamDB_Init = p_StreamDB_Init.RCast<void (*)(const char*)>(); /*48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 41 54 41 56 41 57 48 83 EC 40 48 8B E9*/

		p_RTech_FindFreeSlotInFiles = g_GameDll.FindPatternSIMD("44 8B 51 0C 4C 8B C1");
		RTech_FindFreeSlotInFiles   = p_RTech_FindFreeSlotInFiles.RCast<int32_t(*)(int32_t*)>(); /*44 8B 51 0C 4C 8B C1*/

		p_RTech_RegisterAsset = g_GameDll.FindPatternSIMD("4D 89 42 08").FindPatternSelf("48 89 6C", CMemory::Direction::UP);
		RTech_RegisterAsset   = p_RTech_RegisterAsset.RCast<void(*)(int, int, const char*, void*, void*, void*, void*, int, int, uint32_t, int, int)>(); /*4D 89 42 08*/
	}
	virtual void GetVar(void) const
	{
		s_pFileArray      = p_StreamDB_Init.Offset(0x70).FindPatternSelf("48 8D 0D", CMemory::Direction::DOWN, 512, 2).ResolveRelativeAddress(0x3, 0x7).RCast<int32_t*>();
		s_pFileHandles    = p_StreamDB_Init.Offset(0x70).FindPatternSelf("4C 8D", CMemory::Direction::DOWN, 512, 1).ResolveRelativeAddress(0x3, 0x7).RCast<pFileHandleTracker_t*>();
		s_pFileArrayMutex = p_StreamDB_Init.Offset(0x70).FindPatternSelf("48 8D 0D", CMemory::Direction::DOWN, 512, 1).ResolveRelativeAddress(0x3, 0x7).RCast<PSRWLOCK*>();

		g_pLoadedPakInfo     = p_Pak_UnloadPak.FindPattern("48 8D 05", CMemory::Direction::DOWN).ResolveRelativeAddressSelf(0x3, 0x7).RCast<PakLoadedInfo_t*>();
		g_pRequestedPakCount = p_Pak_UnloadPak.FindPattern("66 89", CMemory::Direction::DOWN, 450).ResolveRelativeAddressSelf(0x3, 0x7).RCast<int16_t*>();
		g_pLoadedPakCount    = &*g_pRequestedPakCount - 1; // '-1' shifts it back with sizeof(int16_t).

		g_pPakGlobals   = g_GameDll.FindPatternSIMD("48 8D 1D ?? ?? ?? ?? 45 8D 5A 0E").ResolveRelativeAddressSelf(0x3, 0x7).RCast<PakGlobals_t*>(); /*48 8D 1D ? ? ? ? 45 8D 5A 0E*/
		g_pPakLoadJobID = reinterpret_cast<JobID_t*>(&*g_pLoadedPakCount - 2);

		g_pPakFifoLock         = p_JT_HelpWithAnything.Offset(0x155).FindPatternSelf("48 8D 0D").ResolveRelativeAddressSelf(0x3, 0x7).RCast<JobFifoLock_s*>();
		g_pPakFifoLockWrapper  = p_JT_HelpWithAnything.Offset(0x1BC).FindPatternSelf("48 8D 0D").ResolveRelativeAddressSelf(0x3, 0x7).RCast<void*>();
		g_bPakFifoLockAcquired = p_JT_HelpWithAnything.Offset(0x50).FindPatternSelf("C6 05").ResolveRelativeAddressSelf(0x2, 0x7).RCast<bool*>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
