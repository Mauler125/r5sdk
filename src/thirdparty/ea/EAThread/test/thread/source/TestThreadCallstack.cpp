///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "TestThread.h"
#include <EATest/EATest.h>
#include <eathread/eathread.h>
#include <eathread/eathread_callstack.h>
#include <eathread/eathread_callstack_context.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/fixed_string.h>
#include <EATest/EATest.h>
#include <EAStdC/EAStopwatch.h>
#include <EAStdC/EASprintf.h>
#include <EASTL/set.h>
#include <eathread/eathread_thread.h>
#include <eathread/eathread_sync.h>
#include <eathread/eathread_semaphore.h>


#ifdef _MSC_VER
#pragma warning(push, 0)
#include <Windows.h>
#endif


int TestThreadCallstack()
{
	int nErrorCount(0);


	#if defined(EA_PLATFORM_MICROSOFT)
		// bool ThreadHandlesAreEqual(intptr_t threadId1, intptr_t threadId2);
		// uint32_t GetThreadIdFromThreadHandle(intptr_t threadId);
	#endif

	// To do: Implement tests for the following for supporting platforms.
	// bool GetCallstackContext(CallstackContext& context, intptr_t threadId = 0);
	// bool GetCallstackContextSysThreadId(CallstackContext& context, intptr_t sysThreadId = 0);
	// void GetCallstackContext(CallstackContext& context, const Context* pContext = NULL);
	// size_t GetModuleFromAddress(const void* pAddress, char* pModuleFileName, size_t moduleNameCapacity);
	// ModuleHandle GetModuleHandleFromAddress(const void* pAddress);

	// EA::Thread::CallstackContext context;
	// EA::Thread::GetCallstackContext(context, EA::Thread::GetThreadId());
	// EATEST_VERIFY(context.mRIP != 0);
	// EATEST_VERIFY(context.mRSP != 0);

	// To consider: Test SetStackBase. It's not simple because SetStackBase is a backup for 
	// when GetStackBase's default functionality doesn't happen to work.
	// void  SetStackBase(void* pStackBase);
	// void  SetStackBase(uintptr_t pStackBase){ SetStackBase((void*)pStackBase); }

	void* pStackBase  = EA::Thread::GetStackBase();
	void* pStackLimit = EA::Thread::GetStackLimit();

	if(pStackBase && pStackLimit)
	{
		EATEST_VERIFY((uintptr_t)&nErrorCount < (uintptr_t)pStackBase);
		EATEST_VERIFY((uintptr_t)&nErrorCount > (uintptr_t)pStackLimit);
	}

	return nErrorCount;
}
