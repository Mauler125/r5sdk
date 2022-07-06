#ifndef THREADTOOLS_H
#define THREADTOOLS_H

inline bool ThreadInterlockedAssignIf(LONG volatile* p, int32 value, int32 comperand)
{
	Assert((size_t)p % 4 == 0);
	return _InterlockedCompareExchange(p, comperand, value);
}
inline void ThreadSleep(unsigned nMilliseconds)
{
#ifdef _WIN32

#ifdef PLATFORM_WINDOWS_PC // This is performed by the game module!
	//static bool bInitialized = false;
	//if (!bInitialized)
	//{
	//	bInitialized = true;
	//	// Set the timer resolution to 1 ms (default is 10.0, 15.6, 2.5, 1.0 or
	//	// some other value depending on hardware and software) so that we can
	//	// use Sleep( 1 ) to avoid wasting CPU time without missing our frame
	//	// rate.
	//	timeBeginPeriod(1);
	//}
#endif

	Sleep(nMilliseconds);
#elif PLATFORM_PS3
	if (nMilliseconds == 0)
	{
		// sys_ppu_thread_yield doesn't seem to function properly, so sleep instead.
		sys_timer_usleep(60);
	}
	else
	{
		sys_timer_usleep(nMilliseconds * 1000);
	}
#elif defined(POSIX)
	usleep(nMilliseconds * 1000);
#endif
}
//=============================================================================
class CThreadFastMutex;

inline CMemory p_MutexInternal_WaitForLock;
inline auto v_MutexInternal_WaitForLock = p_MutexInternal_WaitForLock.RCast<int (*)(CThreadFastMutex* mutex)>();

inline CMemory p_MutexInternal_ReleaseWaiter;
inline auto v_MutexInternal_ReleaseWaiter = p_MutexInternal_ReleaseWaiter.RCast<int (*)(CThreadFastMutex* mutex)>();

///////////////////////////////////////////////////////////////////////////////
class CThreadFastMutex
{
public:
	int WaitForLock(void) {
		return v_MutexInternal_WaitForLock(this);
	}
	int ReleaseWaiter(void) {
		return v_MutexInternal_ReleaseWaiter(this);
	}

	inline uint32  GetOwnerId(void)   const { return m_nOwnerID; }
	inline int     GetDepth(void)     const { return m_nDepth; }
	inline int     GetAddend(void)    const { return m_lAddend; }
	inline HANDLE  GetSemaphore(void) const { return m_hSemaphore; }

private:
	volatile uint32_t m_nOwnerID;
	int               m_nDepth;
	volatile int      m_lAddend;
	HANDLE            m_hSemaphore;
};

///////////////////////////////////////////////////////////////////////////////
class VThreadTools : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: CThreadFastMutex::WaitForLock        : {:#18x} |\n", p_MutexInternal_WaitForLock.GetPtr());
		spdlog::debug("| FUN: CThreadFastMutex::ReleaseWaiter      : {:#18x} |\n", p_MutexInternal_ReleaseWaiter.GetPtr());
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		p_MutexInternal_WaitForLock   = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x20\x48\x8B\xD9\xFF\x15\x00\x00\x00\x00"), "xxxx?xxxx?xxxxxxxxxx????");
		p_MutexInternal_ReleaseWaiter = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x20\x8B\x41\x04\x48\x8B\xD9\x83\xE8\x01"), "xxxxxxxxxxxxxxx");

		v_MutexInternal_WaitForLock   = p_MutexInternal_WaitForLock.RCast<int (*)(CThreadFastMutex*)>();   /*48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 48 8B D9 FF 15 ?? ?? ?? ??*/
		v_MutexInternal_ReleaseWaiter = p_MutexInternal_ReleaseWaiter.RCast<int (*)(CThreadFastMutex*)>(); /*40 53 48 83 EC 20 8B 41 04 48 8B D9 83 E8 01*/
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VThreadTools);

#endif // THREADTOOLS_H
