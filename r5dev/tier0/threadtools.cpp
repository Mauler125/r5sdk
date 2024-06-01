//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Thread tools
//
// $Workfile: $
// $NoKeywords: $
//===========================================================================//

#include "tier0/threadtools.h"
#include "tier0/jobthread.h"

#define INIT_SEM_COUNT 0
#define MAX_SEM_COUNT 1

int CThreadFastMutex::Lock(void)
{
	const DWORD threadId = GetCurrentThreadId();
	LONG result = ThreadInterlockedCompareExchange((volatile LONG*)&m_lAddend, 0, 1);

	if (result)
	{
		if (m_nOwnerID == threadId)
		{
			return ++m_nDepth;
		}

		LONG cycle = 1;

		while (true)
		{
			LONG64 delay = (10 * cycle);
			if (delay)
			{
				do
				{
					ThreadPause();
					--delay;
				} while (delay);
			}

			result = ThreadInterlockedCompareExchange((volatile LONG*)&m_lAddend, 0, 1);

			if (!result)
				break;

			if (++cycle > 5)
			{
				if (_InterlockedIncrement((volatile LONG*)&m_lAddend) != 1)
				{
					if (!m_hSemaphore)
					{
						HANDLE hSemaphore = CreateSemaphoreA(
							NULL, INIT_SEM_COUNT, MAX_SEM_COUNT, NULL);

						if (ThreadInterlockedCompareExchange64(
							(volatile LONG64*)&m_hSemaphore, NULL, (LONG64)hSemaphore))
							CloseHandle(hSemaphore);
					}
					WaitForSingleObject(m_hSemaphore, INFINITE);
				}
				m_nOwnerID = threadId;
				m_nDepth = 1;

				return m_nDepth;
			}
		}
	}

	m_nDepth = 1;
	m_nOwnerID = threadId;

	return result;
}

int CThreadFastMutex::Unlock()
{
	LONG result = m_nDepth - 1;
	m_nDepth = result;

	if (!result)
	{
		m_nOwnerID = 0;
		result = _InterlockedExchangeAdd((volatile LONG*)&m_lAddend, 0xFFFFFFFF);

		if (result != 1)
		{
			if (!m_hSemaphore)
			{
				const HANDLE SemaphoreA = CreateSemaphoreA(NULL, INIT_SEM_COUNT, MAX_SEM_COUNT, NULL);

				if (ThreadInterlockedAssignIf64((volatile LONG64*)&m_hSemaphore, NULL, (LONG64)SemaphoreA))
					CloseHandle(SemaphoreA);
			}

			return ReleaseSemaphore(m_hSemaphore, 1, NULL);
		}
	}
	return result;
}

void CThreadSpinRWLock::SpinLockForWrite()
{
	int i;
	if (TryLockForWrite_UnforcedInline())
	{
		return;
	}

	for (i = THREAD_SPIN; i != 0; --i)
	{
		if (TryLockForWrite_UnforcedInline())
		{
			return;
		}
		ThreadPause();
	}

	for (i = THREAD_SPIN; i != 0; --i)
	{
		if (TryLockForWrite_UnforcedInline())
		{
			return;
		}
		ThreadPause();
		if (i % 1024 == 0)
		{
			ThreadSleep(0);
		}
	}

	for (i = THREAD_SPIN * 4; i != 0; --i)
	{
		if (TryLockForWrite_UnforcedInline())
		{
			return;
		}

		ThreadPause();
		ThreadSleep(0);
	}

	for (;; ) // coded as for instead of while to make easy to breakpoint success
	{
		if (TryLockForWrite_UnforcedInline())
		{
			return;
		}

		ThreadPause();
		ThreadSleep(1);
	}
}

void CThreadSpinRWLock::SpinLockForRead()
{
	int i;
	for (i = THREAD_SPIN; i != 0; --i)
	{
		if (TryLockForRead_UnforcedInline())
		{
			return;
		}
		ThreadPause();
	}

	for (i = THREAD_SPIN; i != 0; --i)
	{
		if (TryLockForRead_UnforcedInline())
		{
			return;
		}
		ThreadPause();
		if (i % 1024 == 0)
		{
			ThreadSleep(0);
		}
	}

	for (i = THREAD_SPIN * 4; i != 0; --i)
	{
		if (TryLockForRead_UnforcedInline())
		{
			return;
		}

		ThreadPause();
		ThreadSleep(0);
	}

	for (;; ) // coded as for instead of while to make easy to breakpoint success
	{
		if (TryLockForRead_UnforcedInline())
		{
			return;
		}

		ThreadPause();
		ThreadSleep(1);
	}
}

bool ThreadInMainThread()
{
	return (ThreadGetCurrentId() == (*g_ThreadMainThreadID));
}

bool ThreadInServerFrameThread()
{
	return (ThreadGetCurrentId() == (*g_ThreadServerFrameThreadID) 
		&& JT_GetCurrentJob() == (*g_CurrentServerFrameJobID));
}

bool ThreadInMainOrServerFrameThread()
{
	return (ThreadInMainThread() || ThreadInServerFrameThread());
}

bool ThreadCouldDoServerWork()
{
	if (*g_ThreadServerFrameThreadID == -1)
		return ThreadInMainThread();

	return ThreadInServerFrameThread();
}

void ThreadJoinServerJob()
{
	if (ThreadCouldDoServerWork())
		return; // No job to join

	if (*g_AllocatedServerFrameJobID)
	{
		JT_WaitForJobAndOnlyHelpWithJobTypes(*g_AllocatedServerFrameJobID, NULL, 0xFFFFFFFFFFFFFFFF);
		*g_AllocatedServerFrameJobID = 0;
	}
}

// NOTE: originally the game exported 'ThreadInMainThread()' and ThreadInServerFrameThread(),
// but since the game is built static, and all instances of said functions are inline, we had
// to export the variable symbols instead and get them here to reimplement said functions.
ThreadId_t* g_ThreadMainThreadID = CModule::GetExportedSymbol(CModule::GetProcessEnvironmentBlock()->ImageBaseAddress, "g_ThreadMainThreadID").RCast<ThreadId_t*>();
ThreadId_t* g_ThreadServerFrameThreadID = CModule::GetExportedSymbol(CModule::GetProcessEnvironmentBlock()->ImageBaseAddress, "g_ThreadServerFrameThreadID").RCast<ThreadId_t*>();
