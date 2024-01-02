#pragma once
#include "tier0/jobthread.h"
#include "vpklib/packedstore.h"
#include "rtech/rtech_game.h"
#include "public/rendersystem/schema/texture.g.h"
#include "public/rtech/ipakfile.h"

/* ==== RTECH =========================================================================================================================================================== */
// [ PIXIE ]: I'm very unsure about this, but it really seems like it
inline int32_t(*RTech_FindFreeSlotInFiles)(int32_t*);
inline void(*RTech_RegisterAsset)(int, int, const char*, void*, void*, void*, void*, int, int, uint32_t, int, int);
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
		LogFunAdr("RTech::FindFreeSlotInFiles", RTech_FindFreeSlotInFiles);

		LogFunAdr("StreamDB_Init", v_StreamDB_Init);
		LogVarAdr("s_FileArray", s_pFileArray);
		LogVarAdr("s_FileArrayMutex", s_pFileArrayMutex);
		LogVarAdr("s_FileHandles", s_pFileHandles);

		LogVarAdr("g_loadedPakInfo", g_pLoadedPakInfo);
		LogVarAdr("g_loadedPakCount", g_pLoadedPakCount);
		LogVarAdr("g_requestedPakCount", g_pRequestedPakCount);

		LogVarAdr("g_pakGlobals", g_pPakGlobals);
		LogVarAdr("g_pakLoadJobID", g_pPakLoadJobID);

		LogVarAdr("g_pakFifoLock", g_pPakFifoLock);
		LogVarAdr("g_pakFifoLockWrapper", g_pPakFifoLockWrapper);
		LogVarAdr("g_pakFifoLockAcquired", g_bPakFifoLockAcquired);
	}
	virtual void GetFun(void) const 
	{
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 41 54 41 56 41 57 48 83 EC 40 48 8B E9").GetPtr(v_StreamDB_Init);
		g_GameDll.FindPatternSIMD("44 8B 51 0C 4C 8B C1").GetPtr(RTech_FindFreeSlotInFiles);
		g_GameDll.FindPatternSIMD("4D 89 42 08").FindPatternSelf("48 89 6C", CMemory::Direction::UP).GetPtr(RTech_RegisterAsset);
	}
	virtual void GetVar(void) const
	{
		s_pFileArray      = CMemory(v_StreamDB_Init).Offset(0x70).FindPatternSelf("48 8D 0D", CMemory::Direction::DOWN, 512, 2).ResolveRelativeAddress(0x3, 0x7).RCast<int32_t*>();
		s_pFileHandles    = CMemory(v_StreamDB_Init).Offset(0x70).FindPatternSelf("4C 8D", CMemory::Direction::DOWN, 512, 1).ResolveRelativeAddress(0x3, 0x7).RCast<pFileHandleTracker_t*>();
		s_pFileArrayMutex = CMemory(v_StreamDB_Init).Offset(0x70).FindPatternSelf("48 8D 0D", CMemory::Direction::DOWN, 512, 1).ResolveRelativeAddress(0x3, 0x7).RCast<PSRWLOCK*>();

		g_pLoadedPakInfo     = CMemory(v_Pak_UnloadPak).FindPattern("48 8D 05", CMemory::Direction::DOWN).ResolveRelativeAddressSelf(0x3, 0x7).RCast<PakLoadedInfo_t*>();
		g_pRequestedPakCount = CMemory(v_Pak_UnloadPak).FindPattern("66 89", CMemory::Direction::DOWN, 450).ResolveRelativeAddressSelf(0x3, 0x7).RCast<int16_t*>();
		g_pLoadedPakCount    = &*g_pRequestedPakCount - 1; // '-1' shifts it back with sizeof(int16_t).

		g_pPakGlobals   = g_GameDll.FindPatternSIMD("48 8D 1D ?? ?? ?? ?? 45 8D 5A 0E").ResolveRelativeAddressSelf(0x3, 0x7).RCast<PakGlobals_t*>(); /*48 8D 1D ? ? ? ? 45 8D 5A 0E*/
		g_pPakLoadJobID = reinterpret_cast<JobID_t*>(&*g_pLoadedPakCount - 2);

		g_pPakFifoLock         = CMemory(JT_HelpWithAnything).Offset(0x155).FindPatternSelf("48 8D 0D").ResolveRelativeAddressSelf(0x3, 0x7).RCast<JobFifoLock_s*>();
		g_pPakFifoLockWrapper  = CMemory(JT_HelpWithAnything).Offset(0x1BC).FindPatternSelf("48 8D 0D").ResolveRelativeAddressSelf(0x3, 0x7).RCast<void*>();
		g_bPakFifoLockAcquired = CMemory(JT_HelpWithAnything).Offset(0x50).FindPatternSelf("C6 05").ResolveRelativeAddressSelf(0x2, 0x7).RCast<bool*>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
