#ifndef JOBTHREAD_H
#define JOBTHREAD_H

struct JobFifoLock_s
{
};

typedef uint32_t JobID_t;

inline void(*JT_ParallelCall)(void);
inline void*(*JT_HelpWithAnything)(bool bShouldLoadPak);
inline bool(*JT_AcquireFifoLock)(struct JobFifoLock_s* pFifo);
inline void(*JT_ReleaseFifoLock)(struct JobFifoLock_s* pFifo);

///////////////////////////////////////////////////////////////////////////////
class VJobThread : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("JT_ParallelCall", JT_ParallelCall);
		LogFunAdr("JT_HelpWithAnything", JT_HelpWithAnything);
		LogFunAdr("JT_AcquireFifoLock", JT_AcquireFifoLock);
		LogFunAdr("JT_ReleaseFifoLock", JT_ReleaseFifoLock);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 8B C4 48 89 58 08 48 89 78 10 55 48 8D 68 A1 48 81 EC ?? ?? ?? ?? 0F 29 70 E8 48 8D 1D ?? ?? ?? ??").GetPtr(JT_ParallelCall);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 30 80 3D ?? ?? ?? ?? ??").GetPtr(JT_HelpWithAnything);
		g_GameDll.FindPatternSIMD("48 83 EC 08 65 48 8B 04 25 ?? ?? ?? ?? 4C 8B C1").GetPtr(JT_AcquireFifoLock);
		g_GameDll.FindPatternSIMD("48 83 EC 28 44 8B 11").GetPtr(JT_ReleaseFifoLock);
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////

#endif // JOBTHREAD_H
