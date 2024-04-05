#ifndef RTECH_PAKSTATE_H
#define RTECH_PAKSTATE_H
#include "tier0/tslist.h"
#include "tier0/jobthread.h"
#include "launcher/launcher.h"

#include "rtech/ipakfile.h"

inline PakGlobals_t* g_pPakGlobals;
inline PakLoadedInfo_t* g_pLoadedPakInfo;

inline JobID_t* g_pPakLoadJobID;

inline int16_t* g_pLoadedPakCount;
inline int16_t* g_pRequestedPakCount;

inline JobFifoLock_s* g_pPakFifoLock;
inline void* g_pPakFifoLockWrapper; // Pointer to functor that takes the global pak fifolock as argument.
inline bool* g_bPakFifoLockAcquired;

// bool as int64
inline int64_t* g_pPakHasPendingUnloadJobs;

///////////////////////////////////////////////////////////////////////////////
class V_PakState : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogVarAdr("g_pakGlobals", g_pPakGlobals);
		LogVarAdr("g_loadedPakInfo", g_pLoadedPakInfo);

		LogVarAdr("g_pakLoadJobID", g_pPakLoadJobID);

		LogVarAdr("g_loadedPakCount", g_pLoadedPakCount);
		LogVarAdr("g_requestedPakCount", g_pRequestedPakCount);

		LogVarAdr("g_pakFifoLock", g_pPakFifoLock);
		LogVarAdr("g_pakFifoLockWrapper", g_pPakFifoLockWrapper);
		LogVarAdr("g_pakFifoLockAcquired", g_bPakFifoLockAcquired);

		LogVarAdr("g_pakHasPendingUnloadJobs", g_pPakHasPendingUnloadJobs);
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const
	{
		extern void(*v_Pak_UnloadAsync)(PakHandle_t);
		const CMemory pakUnloadBase(v_Pak_UnloadAsync);

		g_pLoadedPakInfo = pakUnloadBase.FindPattern("48 8D 05", CMemory::Direction::DOWN).ResolveRelativeAddressSelf(0x3, 0x7).RCast<PakLoadedInfo_t*>();
		g_pRequestedPakCount = pakUnloadBase.FindPattern("66 89", CMemory::Direction::DOWN, 450).ResolveRelativeAddressSelf(0x3, 0x7).RCast<int16_t*>();
		g_pLoadedPakCount = &*g_pRequestedPakCount - 1; // '-1' shifts it back with sizeof(int16_t).

		g_pPakGlobals = g_GameDll.FindPatternSIMD("48 8D 1D ?? ?? ?? ?? 45 8D 5A 0E").ResolveRelativeAddressSelf(0x3, 0x7).RCast<PakGlobals_t*>(); /*48 8D 1D ? ? ? ? 45 8D 5A 0E*/
		g_pPakLoadJobID = reinterpret_cast<JobID_t*>(&*g_pLoadedPakCount - 2);

		const CMemory jtBase(JT_HelpWithAnything);

		g_pPakFifoLock = jtBase.Offset(0x155).FindPatternSelf("48 8D 0D").ResolveRelativeAddressSelf(0x3, 0x7).RCast<JobFifoLock_s*>();
		g_pPakFifoLockWrapper = jtBase.Offset(0x1BC).FindPatternSelf("48 8D 0D").ResolveRelativeAddressSelf(0x3, 0x7).RCast<void*>();
		g_bPakFifoLockAcquired = jtBase.Offset(0x50).FindPatternSelf("C6 05").ResolveRelativeAddressSelf(0x2, 0x7).RCast<bool*>();

		extern EPakStatus(*v_Pak_WaitAsync)(PakHandle_t, void*);
		const CMemory pakWaitBase(v_Pak_WaitAsync);

		pakWaitBase.Offset(0x80).FindPatternSelf("48 39").ResolveRelativeAddressSelf(3, 7).GetPtr(g_pPakHasPendingUnloadJobs);
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const { };
};
///////////////////////////////////////////////////////////////////////////////


#endif // RTECH_PAKSTATE_H
