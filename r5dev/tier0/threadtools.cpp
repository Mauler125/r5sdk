//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Thread tools
//
// $Workfile: $
// $NoKeywords: $
//===========================================================================//

#include "tier0/threadtools.h"

#define INIT_SEM_COUNT 0
#define MAX_SEM_COUNT 1

int CThreadFastMutex::Lock(void)
{
	DWORD threadId = GetCurrentThreadId();
	LONG result = ThreadInterlockedCompareExchange((volatile LONG*)&m_lAddend, 0, 1);

	if (result)
	{
		if (m_nOwnerID == threadId)
		{
			result = m_nDepth + 1;
			m_nDepth = result;

			return result;
		}

		LONG cycle = 1;
		LONG64 delay;

		while (true)
		{
			delay = (10 * cycle);
			if (delay)
			{
				do
				{
					ThreadPause();
					--delay;
				} while (delay);
			}

			result = ThreadInterlockedCompareExchange((volatile LONG*)&m_lAddend, 0, 1);

			if (result)
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
	LONG result; // eax
	HANDLE SemaphoreA; // rcx

	result = m_nDepth - 1;
	m_nDepth = result;

	if (!result)
	{
		m_nOwnerID = 0;
		result = _InterlockedExchangeAdd((volatile LONG*)&m_lAddend, 0xFFFFFFFF);

		if (result != 1)
		{
			if (!m_hSemaphore)
			{
				SemaphoreA = CreateSemaphoreA(NULL, INIT_SEM_COUNT, MAX_SEM_COUNT, NULL);

				if (ThreadInterlockedAssignIf64(
					(volatile LONG64*)&m_hSemaphore, NULL, (LONG64)SemaphoreA))
					CloseHandle(SemaphoreA);
			}
			return ReleaseSemaphore(m_hSemaphore, 1, NULL);
		}
	}
	return result;
}
