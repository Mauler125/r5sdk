#ifndef RTECH_PAKSTATE_H
#define RTECH_PAKSTATE_H
#include "tier0/tslist.h"
#include "tier0/jobthread.h"
#include "launcher/launcher.h"

#include "rtech/ipakfile.h"

inline PakGlobals_s* g_pakGlobals;
inline JobHelpCallback_t g_pPakFifoLockWrapper; // Pointer to functor that takes the global pak fifolock as argument.

// TODO: rename to 'g_bPakFifoLockAcquiredInMainThread'
// if this is set, JT_ReleaseFifoLock has to be called
// twice as the depth goes up to the thread that
// acquired the lock + the main thread
inline bool* g_bPakFifoLockAcquired;

///////////////////////////////////////////////////////////////////////////////
class V_PakState : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogVarAdr("g_pakGlobals", g_pakGlobals);

		LogVarAdr("g_pakFifoLockWrapper", g_pPakFifoLockWrapper);
		LogVarAdr("g_pakFifoLockAcquired", g_bPakFifoLockAcquired);
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const
	{
		g_pakGlobals = g_GameDll.FindPatternSIMD("48 8D 1D ?? ?? ?? ?? 45 8D 5A 0E").ResolveRelativeAddressSelf(0x3, 0x7).RCast<PakGlobals_s*>(); /*48 8D 1D ? ? ? ? 45 8D 5A 0E*/

		const CMemory jtBase(JT_HelpWithAnything);

		g_pPakFifoLockWrapper = jtBase.Offset(0x1BC).FindPatternSelf("48 8D 0D").ResolveRelativeAddressSelf(0x3, 0x7).RCast<JobHelpCallback_t>();
		g_bPakFifoLockAcquired = jtBase.Offset(0x50).FindPatternSelf("C6 05").ResolveRelativeAddressSelf(0x2, 0x7).RCast<bool*>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const { };
};
///////////////////////////////////////////////////////////////////////////////


#endif // RTECH_PAKSTATE_H
