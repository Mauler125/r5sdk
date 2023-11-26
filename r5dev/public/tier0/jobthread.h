#ifndef JOBTHREAD_H
#define JOBTHREAD_H

struct JobFifoLock_s
{
};

typedef uint32_t JobID_t;

inline CMemory p_JT_ParallelCall;
inline void(*JT_ParallelCall)(void);

inline CMemory p_JT_HelpWithAnything;
inline void*(*JT_HelpWithAnything)(bool bShouldLoadPak);

inline CMemory p_JT_AcquireFifoLock;
inline bool(*JT_AcquireFifoLock)(struct JobFifoLock_s* pFifo);

inline CMemory p_JT_ReleaseFifoLock;
inline void(*JT_ReleaseFifoLock)(struct JobFifoLock_s* pFifo);

///////////////////////////////////////////////////////////////////////////////
class VJobThread : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("JT_ParallelCall", p_JT_ParallelCall.GetPtr());
		LogFunAdr("JT_HelpWithAnything", p_JT_HelpWithAnything.GetPtr());
		LogFunAdr("JT_AcquireFifoLock", p_JT_AcquireFifoLock.GetPtr());
		LogFunAdr("JT_ReleaseFifoLock", p_JT_ReleaseFifoLock.GetPtr());
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_JT_ParallelCall     = g_GameDll.FindPatternSIMD("48 8B C4 48 89 58 10 48 89 70 18 55 57 41 57");
		p_JT_HelpWithAnything = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 41 56 48 83 EC 30 80 3D ?? ?? ?? ?? ??");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_JT_ParallelCall     = g_GameDll.FindPatternSIMD("48 8B C4 48 89 58 08 48 89 78 10 55 48 8D 68 A1 48 81 EC ?? ?? ?? ?? 0F 29 70 E8 48 8D 1D ?? ?? ?? ??");
		p_JT_HelpWithAnything = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 30 80 3D ?? ?? ?? ?? ??");
#endif
		p_JT_AcquireFifoLock = g_GameDll.FindPatternSIMD("48 83 EC 08 65 48 8B 04 25 ?? ?? ?? ?? 4C 8B C1");
		p_JT_ReleaseFifoLock = g_GameDll.FindPatternSIMD("48 83 EC 28 44 8B 11");

		JT_ParallelCall = p_JT_ParallelCall.RCast<void (*)(void)>();                         /*48 8B C4 48 89 58 08 48 89 78 10 55 48 8D 68 A1 48 81 EC ?? ?? ?? ?? 0F 29 70 E8 48 8D 1D ?? ?? ?? ??*/
		JT_HelpWithAnything = p_JT_HelpWithAnything.RCast<void* (*)(bool bShouldLoadPak)>(); /*48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 30 80 3D ?? ?? ?? ?? ??*/
		JT_AcquireFifoLock = p_JT_AcquireFifoLock.RCast<bool (*)(struct JobFifoLock_s*)>();  /*48 83 EC 08 65 48 8B 04 25 ?? ?? ?? ?? 4C 8B C1*/
		JT_ReleaseFifoLock = p_JT_ReleaseFifoLock.RCast<void (*)(struct JobFifoLock_s*)>();  /*48 83 EC 28 44 8B 11*/
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////

#endif // JOBTHREAD_H
