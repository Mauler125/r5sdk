///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include <EATest/EATest.h>
#include "TestThread.h"
#include <eathread/eathread_thread.h>
#include <eathread/eathread_semaphore.h>
#include <eathread/eathread_sync.h>
#include <eathread/eathread_atomic.h>

EA_DISABLE_ALL_VC_WARNINGS()
#include <string.h>
EA_RESTORE_ALL_VC_WARNINGS()

using namespace EA::Thread;
using namespace EA::Thread::detail;
using namespace EA::UnitTest;

//-------------------------------------------------------------------------------
// Globals
static Semaphore gSemaphore;

//-------------------------------------------------------------------------------
//
static intptr_t TestFunction1(void*)
{
	// Wait until we are signaled by the unit test to complete.
	gSemaphore.Wait();
	return 0;
}

//-------------------------------------------------------------------------------
//
int TestSimpleEnumerateThreads()
{
	int nErrorCount = 0;
	static EA::Thread::AtomicInt<size_t> snThreadStartCount;
	snThreadStartCount = 0;

	auto threadEntry = [](void*) -> intptr_t
	{
		snThreadStartCount++;

		// Wait until we are signaled by the unit test to complete.
		gSemaphore.Wait();
		return 0;
	};

	const size_t kMaxTestThreadEnumCount = 16;
	ThreadEnumData enumData[kMaxTestThreadEnumCount];

	// Prevents all threads from returning.    
	gSemaphore.Init(0); 

	// Startup all the threads we want to monitor.
	Thread threads[kMaxTestThreadEnumCount];
	for(size_t i = 0; i < kMaxTestThreadEnumCount; i++) 
	{        
		threads[i].Begin(threadEntry);
	}

	// Give all the threads a chance to start up. 
	while(snThreadStartCount != kMaxTestThreadEnumCount)
		EA::Thread::ThreadSleep(0);  

	// Enumerate the active threads
	size_t threadCount = EA::Thread::EnumerateThreads(enumData, EAArrayCount(enumData));
	EATEST_VERIFY_MSG(threadCount >= kMaxTestThreadEnumCount, "Incorrect number of threads reported.");
	// Report("Enumerated (at least) %d threads. Found (%d).\n", kMaxTestThreadEnumCount, threadCount);

	for(size_t j = 0; j < kMaxTestThreadEnumCount; j++)
	{   
		//Report("\tThread id: %s\n", ThreadIdToStringBuffer(enumData[j].mpThreadDynamicData->mhThread).c_str());
		if(enumData[j].mpThreadDynamicData == NULL)
			continue;

	#if !defined(EA_PLATFORM_NX)  // mpStackBase is null because the necessary POSIX API (pthread_getattr_np) isn't implemented by Nintendo
		if(strcmp(enumData[j].mpThreadDynamicData->mName, "external") != 0) // Disabled because we can't guarantee across all platforms that a stack base is available.  This will be fixed in a future release.
		{
			EATEST_VERIFY_MSG(enumData[j].mpThreadDynamicData->mpStackBase != NULL, "All thread meta data is expected to have the stack base address.");
		}
	#endif
		enumData[j].Release(); 
	}
	
	// Signal the threads to complete.
	gSemaphore.Post(kMaxTestThreadEnumCount);

	// Wait for all threads to complete.
	for(size_t i = 0; i < kMaxTestThreadEnumCount; i++)
	{
		if(threads[i].GetStatus() != Thread::kStatusEnded)
			threads[i].WaitForEnd();
	}

	return nErrorCount;
}

//-------------------------------------------------------------------------------
//
int TestSimpleEnumerateThreads_KillThreadsEarly()
{
	int nErrorCount = 0;

	const size_t kMaxTestThreadEnumCount = 16;
	ThreadEnumData enumData[kMaxTestThreadEnumCount];

	// Prevents all threads from returning.    
	gSemaphore.Init(0); 

	// Startup all the threads we want to monitor.
	Thread threads[kMaxTestThreadEnumCount];
	for(size_t i = 0; i < kMaxTestThreadEnumCount; i++) 
	{        
		threads[i].Begin(TestFunction1);       
	}
	EA::Thread::ThreadSleep(300);  // Give all the threads a chance to start up. 

	// Enumerate the active threads
	size_t threadCount = EA::Thread::EnumerateThreads(enumData, EAArrayCount(enumData));
	EATEST_VERIFY_MSG(threadCount >= kMaxTestThreadEnumCount, "Incorrect number of threads reported.");
	// Report("Enumerated (at least) %d threads. Found (%d).\n", kMaxTestThreadEnumCount, threadCount);

	// Signal the threads to complete.
	gSemaphore.Post(kMaxTestThreadEnumCount);
	EA::Thread::ThreadSleep(500);   

	// Terminate the threads before the user explicitly releases them.
	for(size_t i = 0; i < kMaxTestThreadEnumCount; i++)
	{
		if(threads[i].GetStatus() != Thread::kStatusEnded)
			threads[i].WaitForEnd();
	}

	for(size_t j = 0; j < kMaxTestThreadEnumCount; j++)
	{   
		//Report("\tThread id: %s\n", ThreadIdToStringBuffer(enumData[j].mpThreadDynamicData->mhThread).c_str());
		enumData[j].Release(); 
	}

	return nErrorCount;
}

//-------------------------------------------------------------------------------
//
int TestEnumerateThreads_EnumerateMain()
{
	int nErrorCount = 0;

	const size_t kMaxTestThreadEnumCount = 16;
	ThreadEnumData enumData[kMaxTestThreadEnumCount];

	size_t threadCount = EA::Thread::EnumerateThreads(enumData, EAArrayCount(enumData));
	EATEST_VERIFY_MSG(threadCount >= 1, "No threads found.  We are expecting at least the main thread to be reported.");

	int compare_result = strcmp(enumData[0].mpThreadDynamicData->mName, "external");
	EATEST_VERIFY_MSG(compare_result == 0, "Not an externally created thread.");

#if EA_USE_CPP11_CONCURRENCY
	ThreadUniqueId thisThreadId;
	EAThreadGetUniqueId(thisThreadId);

	ThreadUniqueId uniqueThreadId = enumData[0].mpThreadDynamicData->mUniqueThreadId;
	EATEST_VERIFY_MSG(uniqueThreadId == thisThreadId, "Did not return the threadId of this call context.");  
#elif defined(EA_PLATFORM_MICROSOFT) && !EA_POSIX_THREADS_AVAILABLE
	ThreadId enumThreadId = enumData[0].mpThreadDynamicData->mhThread;  // No portable thread id available in the ThreadDynamicDataStructure.
	EATEST_VERIFY_MSG(enumThreadId == EA::Thread::GetThreadId(), "Did not return the threadId of this call context.");  
#else
	ThreadId enumThreadId = enumData[0].mpThreadDynamicData->mThreadId;
	EATEST_VERIFY_MSG(enumThreadId == EA::Thread::GetThreadId(), "Did not return the threadId of this call context.");  
#endif

	return nErrorCount;
}

//-------------------------------------------------------------------------------
//
EA_DISABLE_ALL_VC_WARNINGS()
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>
EA_RESTORE_ALL_VC_WARNINGS()
#include <eathread/eathread_mutex.h>


// TODO(rparolin):  This forces the build-farm to timeout.  Re-enable in the future.
// 
// int TestHeavyLoadThreadRegisteration()
// {
//     int nErrorCount = 0;

// #ifdef EA_PLATFORM_MICROSOFT 
//     // Only tested on Windows because its a reported regression in tools/pipeline related
//     // technologies leveraging a large number of non-eathread threads which would exhaust internal
//     // tracking system.
//     std::atomic<bool> isDone = false;
//     EA::Thread::Mutex s_mutex;

//     {
//         int loopCount = 170; // must exceed the value of EA::Thread::kMaxThreadDynamicDataCount.
//         std::vector<std::thread> threads;
//         while(loopCount--)  
//         {
//             threads.emplace_back(std::thread([&]
//             {
//                 while (!isDone)
//                 {
//                     // We lock an EA::Thread::Mutex because it used to force a 
//                     // non-EAThread thread to be registered due to debug functionality
//                     // requesting a thread id. Verify that locking a mutex no longer requires
//                     // external thread registration by locking more threads that we can track.
//                     EA::Thread::AutoMutex _(s_mutex);
//                 }
//             }));
//         }

//         std::this_thread::sleep_for(std::chrono::milliseconds(100));
//         isDone = true;

//         for (auto& th : threads)
//             th.join();
//     }
// #endif

//     return nErrorCount;
// }


//-------------------------------------------------------------------------------
//
int TestEnumerateThreads()
{
	int nErrorCount = 0;

	nErrorCount += TestSimpleEnumerateThreads();
	nErrorCount += TestSimpleEnumerateThreads_KillThreadsEarly();
	nErrorCount += TestEnumerateThreads_EnumerateMain();
	// nErrorCount += TestHeavyLoadThreadRegisteration();

	return nErrorCount;
}

