///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#ifndef TESTTHREAD_H
#define TESTTHREAD_H


extern unsigned int gTestLengthSeconds;
extern bool IsSuperUser();

// The maximum number of threads spawned during EAThread unit tests.
#ifndef EATHREAD_MAX_CONCURRENT_THREAD_COUNT
	#if defined(EA_PLATFORM_DESKTOP)
		#define EATHREAD_MAX_CONCURRENT_THREAD_COUNT 16
	#elif defined(EA_PLATFORM_MOBILE)
		#define EATHREAD_MAX_CONCURRENT_THREAD_COUNT 4
	#elif defined(EA_PLATFORM_NX)
		#define EATHREAD_MAX_CONCURRENT_THREAD_COUNT 2 // -1 core so we don't kick off the main thread
	#else
		#define EATHREAD_MAX_CONCURRENT_THREAD_COUNT 8
	#endif
#endif


int TestThreadSync();
int TestThreadAtomic();
int TestThreadCallstack();
int TestThreadStorage();
int TestThreadSpinLock();
int TestThreadRWSpinLock();
int TestThreadFutex();
int TestThreadMutex();
int TestThreadRWMutex();
int TestThreadSemaphore();
int TestThreadRWSemaLock();
int TestThreadCondition();
int TestThreadBarrier();
int TestThreadThread();
int TestThreadThreadPool();
int TestThreadSmartPtr();
int TestThreadMisc();
int TestEnumerateThreads();
int TestCpuAvailableAffinityMask();

#endif // Header include guard


