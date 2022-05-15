#ifndef JOBTHREAD_H
#define JOBTHREAD_H

struct JobFifoLock_s
{
};

inline CMemory p_JT_HelpWithAnything;
inline auto JT_HelpWithAnything = p_JT_HelpWithAnything.RCast<void* (*)(bool bShouldLoadPak)>();

inline CMemory p_JT_AcquireFifoLock;
inline auto JT_AcquireFifoLock = p_JT_AcquireFifoLock.RCast<bool (*)(struct JobFifoLock_s* pFifo)>();

inline CMemory p_JT_ReleaseFifoLock;
inline auto JT_ReleaseFifoLock = p_JT_ReleaseFifoLock.RCast<void (*)(struct JobFifoLock_s* pFifo)>();

void JT_Attach();
void JT_Detach();
///////////////////////////////////////////////////////////////////////////////
class VJobThread : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: JT_HelpWithAnything                  : {:#18x} |\n", p_JT_HelpWithAnything.GetPtr());
		spdlog::debug("| FUN: JT_AcquireFifoLock                   : {:#18x} |\n", p_JT_AcquireFifoLock.GetPtr());
		spdlog::debug("| FUN: JT_ReleaseFifoLock                   : {:#18x} |\n", p_JT_ReleaseFifoLock.GetPtr());
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_JT_HelpWithAnything = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x41\x56\x48\x83\xEC\x30\x80\x3D\x00\x00\x00\x00\x00"), "xxxx?xxxx?xxxx?xxxx?xxxxxxxx?????");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_JT_HelpWithAnything = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x30\x80\x3D\x00\x00\x00\x00\x00"), "xxxx?xxxx?xxxxxxx?????");
#endif
		p_JT_AcquireFifoLock = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x83\xEC\x08\x65\x48\x8B\x04\x25\x00\x00\x00\x00\x4C\x8B\xC1"), "xxxxxxxxx????xxx");
		p_JT_ReleaseFifoLock = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x83\xEC\x28\x44\x8B\x11"), "xxxxxxx");

		JT_HelpWithAnything = p_JT_HelpWithAnything.RCast<void* (*)(bool bShouldLoadPak)>(); /*48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 30 80 3D ?? ?? ?? ?? ??*/
		JT_AcquireFifoLock = p_JT_AcquireFifoLock.RCast<bool (*)(struct JobFifoLock_s*)>();  /*48 83 EC 08 65 48 8B 04 25 ?? ?? ?? ?? 4C 8B C1*/
		JT_ReleaseFifoLock = p_JT_ReleaseFifoLock.RCast<void (*)(struct JobFifoLock_s*)>();  /*48 83 EC 28 44 8B 11*/
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VJobThread);

#endif // JOBTHREAD_H
