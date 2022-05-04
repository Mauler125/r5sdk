#ifndef THREADTOOLS_H
#define THREADTOOLS_H
//=============================================================================

inline CMemory p_MutexInternal_WaitForLock;
inline auto v_MutexInternal_WaitForLock = p_MutexInternal_WaitForLock.RCast<int (*)(void* mutex)>();

inline CMemory p_MutexInternal_ReleaseWaiter;
inline auto v_MutexInternal_ReleaseWaiter = p_MutexInternal_ReleaseWaiter.RCast<int (*)(void* mutex)>();

///////////////////////////////////////////////////////////////////////////////
class CThreadFastMutex
{
public:
	static int WaitForLock(CThreadFastMutex* mutex)
	{
		return v_MutexInternal_WaitForLock(mutex);
	}
	static int ReleaseWaiter(CThreadFastMutex* mutex)
	{
		return v_MutexInternal_ReleaseWaiter(mutex);
	}
};

///////////////////////////////////////////////////////////////////////////////
class HThreadTools : public IDetour
{
	virtual void GetAdr(void) const
	{
		std::cout << "| FUN: CThreadFastMutex::WaitForLock        : 0x" << std::hex << std::uppercase << p_MutexInternal_WaitForLock.GetPtr()   << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: CThreadFastMutex::ReleaseWaiter      : 0x" << std::hex << std::uppercase << p_MutexInternal_ReleaseWaiter.GetPtr() << std::setw(nPad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
	virtual void GetFun(void) const
	{
		p_MutexInternal_WaitForLock   = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x20\x48\x8B\xD9\xFF\x15\x00\x00\x00\x00"), "xxxx?xxxx?xxxxxxxxxx????");
		p_MutexInternal_ReleaseWaiter = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x20\x8B\x41\x04\x48\x8B\xD9\x83\xE8\x01"), "xxxxxxxxxxxxxxx");

		v_MutexInternal_WaitForLock   = p_MutexInternal_WaitForLock.RCast<int (*)(void*)>();   /*48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 48 8B D9 FF 15 ?? ?? ?? ??*/
		v_MutexInternal_ReleaseWaiter = p_MutexInternal_ReleaseWaiter.RCast<int (*)(void*)>(); /*40 53 48 83 EC 20 8B 41 04 48 8B D9 83 E8 01*/
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HThreadTools);

#endif // THREADTOOLS_H
