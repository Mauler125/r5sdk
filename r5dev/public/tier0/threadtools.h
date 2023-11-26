#ifndef THREADTOOLS_H
#define THREADTOOLS_H

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
inline void ThreadPause()
{
#if defined( COMPILER_PS3 )
	__db16cyc();
#elif defined( COMPILER_GCC )
	__asm __volatile("pause");
#elif defined ( COMPILER_MSVC64 )
	_mm_pause();
#elif defined( COMPILER_MSVC32 )
	__asm pause;
#elif defined( COMPILER_MSVCX360 )
	YieldProcessor();
	__asm { or r0, r0, r0 }
	YieldProcessor();
	__asm { or r1, r1, r1 }
#else
#error "implement me"
#endif
}

//-----------------------------------------------------------------------------
//
// Interlock methods. These perform very fast atomic thread
// safe operations. These are especially relevant in a multi-core setting.
//
//-----------------------------------------------------------------------------

FORCEINLINE int32 ThreadInterlockedCompareExchange(LONG volatile* pDest, int32 value, int32 comperand)
{
	return _InterlockedCompareExchange(pDest, comperand, value);
}

FORCEINLINE bool ThreadInterlockedAssignIf(LONG volatile* p, int32 value, int32 comperand)
{
	Assert((size_t)p % 4 == 0);
	return _InterlockedCompareExchange(p, comperand, value);
}

FORCEINLINE int64 ThreadInterlockedCompareExchange64(int64 volatile* pDest, int64 value, int64 comperand)
{
	return _InterlockedCompareExchange64(pDest, comperand, value);
}

FORCEINLINE bool ThreadInterlockedAssignIf64(int64 volatile* pDest, int64 value, int64 comperand)
{
	return _InterlockedCompareExchange64(pDest, comperand, value);
}

//-----------------------------------------------------------------------------
//
// Thread checking methods.
//
//-----------------------------------------------------------------------------
#ifndef BUILDING_MATHLIB

inline ThreadId_t* g_ThreadMainThreadID = nullptr;
inline ThreadId_t g_ThreadRenderThreadID = NULL;
inline ThreadId_t* g_ThreadServerFrameThreadID = nullptr;

FORCEINLINE ThreadId_t ThreadGetCurrentId()
{
#ifdef _WIN32
	return GetCurrentThreadId();
#elif defined( _PS3 )
	sys_ppu_thread_t th = 0;
	sys_ppu_thread_get_id(&th);
	return th;
#elif defined(POSIX)
	return (ThreadId_t)pthread_self();
#else
	Assert(0);
	DebuggerBreak();
	return 0;
#endif
}

FORCEINLINE bool ThreadInMainThread()
{
	return (ThreadGetCurrentId() == (*g_ThreadMainThreadID));
}

FORCEINLINE bool ThreadInRenderThread()
{
	return (ThreadGetCurrentId() == g_ThreadRenderThreadID);
}

FORCEINLINE bool ThreadInServerFrameThread()
{
	return (ThreadGetCurrentId() == (*g_ThreadServerFrameThreadID));
}

#endif // !BUILDING_MATHLIB

#ifdef _WIN32
#define NOINLINE
#elif defined( _PS3 )
#define NOINLINE __attribute__ ((noinline))
#elif defined(POSIX)
#define NOINLINE __attribute__ ((noinline))
#endif

#if defined( _X360 ) || defined( _PS3 )
#define ThreadMemoryBarrier() __lwsync()
#elif defined(COMPILER_MSVC)
// Prevent compiler reordering across this barrier. This is
// sufficient for most purposes on x86/x64.
#define ThreadMemoryBarrier() _ReadWriteBarrier()
#elif defined(COMPILER_GCC)
// Prevent compiler reordering across this barrier. This is
// sufficient for most purposes on x86/x64.
// http://preshing.com/20120625/memory-ordering-at-compile-time
#define ThreadMemoryBarrier() asm volatile("" ::: "memory")
#else
#error Every platform needs to define ThreadMemoryBarrier to at least prevent compiler reordering
#endif

//-----------------------------------------------------------------------------
//
// A super-fast thread-safe integer A simple class encapsulating the notion of an 
// atomic integer used across threads that uses the built in and faster 
// "interlocked" functionality rather than a full-blown mutex. Useful for simple 
// things like reference counts, etc.
//
//-----------------------------------------------------------------------------

template <typename T>
class CInterlockedIntT
{
public:
	CInterlockedIntT() : m_value(0) { static_assert((sizeof(T) == sizeof(int32)) || (sizeof(T) == sizeof(int64))); }

	CInterlockedIntT(T value) : m_value(value) {}

	T operator()(void) const { return m_value; }
	operator T() const { return m_value; }

	bool operator!() const { return (m_value == 0); }
	bool operator==(T rhs) const { return (m_value == rhs); }
	bool operator!=(T rhs) const { return (m_value != rhs); }

	T operator++() {
		if (sizeof(T) == sizeof(int32))
			return (T)ThreadInterlockedIncrement((int32*)&m_value);
		else
			return (T)ThreadInterlockedIncrement64((int64*)&m_value);
	}
	T operator++(int) { return operator++() - 1; }

	T operator--() {
		if (sizeof(T) == sizeof(int32))
			return (T)ThreadInterlockedDecrement((int32*)&m_value);
		else
			return (T)ThreadInterlockedDecrement64((int64*)&m_value);
	}

	T operator--(int) { return operator--() + 1; }

	bool AssignIf(T conditionValue, T newValue)
	{
		if V_CONSTEXPR(sizeof(T) == sizeof(int32))
			return ThreadInterlockedAssignIf((LONG*)&m_value, (int32)newValue, (int32)conditionValue);
		else
			return ThreadInterlockedAssignIf64((int64*)&m_value, (int64)newValue, (int64)conditionValue);
	}


	T operator=(T newValue) {
		if (sizeof(T) == sizeof(int32))
			ThreadInterlockedExchange((int32*)&m_value, newValue);
		else
			ThreadInterlockedExchange64((int64*)&m_value, newValue);
		return m_value;
	}

	// Atomic add is like += except it returns the previous value as its return value
	T AtomicAdd(T add) {
		if (sizeof(T) == sizeof(int32))
			return (T)ThreadInterlockedExchangeAdd((int32*)&m_value, (int32)add);
		else
			return (T)ThreadInterlockedExchangeAdd64((int64*)&m_value, (int64)add);
	}


	void operator+=(T add) {
		if (sizeof(T) == sizeof(int32))
			ThreadInterlockedExchangeAdd((int32*)&m_value, (int32)add);
		else
			ThreadInterlockedExchangeAdd64((int64*)&m_value, (int64)add);
	}

	void operator-=(T subtract) { operator+=(-subtract); }
	void operator*=(T multiplier) {
		T original, result;
		do
		{
			original = m_value;
			result = original * multiplier;
		} while (!AssignIf(original, result));
	}
	void operator/=(T divisor) {
		T original, result;
		do
		{
			original = m_value;
			result = original / divisor;
		} while (!AssignIf(original, result));
	}

	T operator+(T rhs) const { return m_value + rhs; }
	T operator-(T rhs) const { return m_value - rhs; }

	T InterlockedExchange(T newValue) {
		if (sizeof(T) == sizeof(int32))
			return (T)ThreadInterlockedExchange((int32*)&m_value, newValue);
		else
			return (T)ThreadInterlockedExchange64((int64*)&m_value, newValue);
	}

private:
	volatile T m_value;
};

typedef CInterlockedIntT<int> CInterlockedInt;
typedef CInterlockedIntT<unsigned> CInterlockedUInt;

#ifndef BUILDING_MATHLIB
//=============================================================================
inline CMemory p_DeclareCurrentThreadIsMainThread;
inline ThreadId_t(*v_DeclareCurrentThreadIsMainThread)(void);

#endif // !BUILDING_MATHLIB

///////////////////////////////////////////////////////////////////////////////
class CThreadFastMutex
{
public:
	int Lock(void);
	int Unlock(void);

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
template <class MUTEX_TYPE = CThreadFastMutex>
class CAutoLockT
{
public:
	FORCEINLINE CAutoLockT(MUTEX_TYPE& lock)
		: m_lock(lock)
	{
		m_lock.Lock();
	}

	FORCEINLINE CAutoLockT(const MUTEX_TYPE& lock)
		: m_lock(const_cast<MUTEX_TYPE&>(lock))
	{
		m_lock.Lock();
	}

	FORCEINLINE ~CAutoLockT()
	{
		m_lock.Unlock();
	}


private:
	MUTEX_TYPE& m_lock;

	// Disallow copying
	CAutoLockT<MUTEX_TYPE>(const CAutoLockT<MUTEX_TYPE>&);
	CAutoLockT<MUTEX_TYPE>& operator=(const CAutoLockT<MUTEX_TYPE>&);
};

///////////////////////////////////////////////////////////////////////////////
template <int size>	struct CAutoLockTypeDeducer {};
template <> struct CAutoLockTypeDeducer<sizeof(CThreadFastMutex)> { typedef CThreadFastMutex Type_t; };

#define AUTO_LOCK_( type, mutex ) \
	CAutoLockT< type > UNIQUE_ID( static_cast<const type &>( mutex ) )

#if defined(__clang__)
#define AUTO_LOCK(mutex) \
	AUTO_LOCK_(typename CAutoLockTypeDeducer<sizeof(mutex)>::Type_t, mutex)
#else
#define AUTO_LOCK(mutex) \
	AUTO_LOCK_(CAutoLockTypeDeducer<sizeof(mutex)>::Type_t, mutex)
#endif

#ifndef BUILDING_MATHLIB
///////////////////////////////////////////////////////////////////////////////
class VThreadTools : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("DeclareCurrentThreadIsMainThread", p_DeclareCurrentThreadIsMainThread.GetPtr());
		LogVarAdr("g_ThreadMainThreadID", reinterpret_cast<uintptr_t>(g_ThreadMainThreadID));
		LogVarAdr("g_ThreadServerFrameThreadID", reinterpret_cast<uintptr_t>(g_ThreadServerFrameThreadID));
	}
	virtual void GetFun(void) const
	{
		p_DeclareCurrentThreadIsMainThread = g_GameDll.FindPatternSIMD("48 83 EC 28 FF 15 ?? ?? ?? ?? 89 05 ?? ?? ?? ?? 48 83 C4 28");
		v_DeclareCurrentThreadIsMainThread = p_DeclareCurrentThreadIsMainThread.RCast<ThreadId_t(*)(void)>(); /*48 83 EC 28 FF 15 ?? ?? ?? ?? 89 05 ?? ?? ?? ?? 48 83 C4 28 */
	}
	virtual void GetVar(void) const
	{
		g_ThreadMainThreadID = p_DeclareCurrentThreadIsMainThread.FindPattern("89 05").ResolveRelativeAddressSelf(0x2, 0x6).RCast<ThreadId_t*>();
		g_ThreadServerFrameThreadID = g_GameDll.FindPatternSIMD("83 79 ?? ?? 75 28 8B").FindPatternSelf("8B 05").ResolveRelativeAddressSelf(0x2, 0x6).RCast<ThreadId_t*>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool /*bAttach*/) const { }
};
///////////////////////////////////////////////////////////////////////////////

#endif // !BUILDING_MATHLIB
#endif // THREADTOOLS_H
