//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Thread tools
//
// $Workfile: $
// $NoKeywords: $
//===========================================================================//

#include "tier0_pch.h"
#include "tier0/threadtools.h"

int32 ThreadInterlockedCompareExchange(LONG volatile* pDest, int32 value, int32 comperand)
{
	return _InterlockedCompareExchange(pDest, comperand, value);
}

bool ThreadInterlockedAssignIf(LONG volatile* p, int32 value, int32 comperand)
{
	Assert((size_t)p % 4 == 0);
	return _InterlockedCompareExchange(p, comperand, value);
}

int64 ThreadInterlockedCompareExchange64(int64 volatile* pDest, int64 value, int64 comperand)
{
	return _InterlockedCompareExchange64(pDest, comperand, value);
}

bool ThreadInterlockedAssignIf64(int64 volatile* pDest, int64 value, int64 comperand)
{
	return _InterlockedCompareExchange64(pDest, comperand, value);
}

bool ThreadInMainThread()
{
	return (ThreadGetCurrentId() == (*g_ThreadMainThreadID));
}

bool ThreadInRenderThread()
{
	return (ThreadGetCurrentId() == g_ThreadRenderThreadID);
}

bool ThreadInServerFrameThread()
{
	return (ThreadGetCurrentId() == (*g_ThreadServerFrameThreadID));
}

ThreadId_t ThreadGetCurrentId()
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