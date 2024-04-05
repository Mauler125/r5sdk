///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "TestThread.h"
#include <EATest/EATest.h>
#include <EAStdC/EAStopwatch.h>
#include <EAStdC/EAString.h>
#include <eathread/eathread_futex.h>
#include <eathread/eathread_thread.h>
#include <eathread/eathread_atomic.h>
#include <eathread/eathread_mutex.h>
#include <stdlib.h>
#include <string.h>

#if defined(EA_PLATFORM_MICROSOFT)
	  #pragma warning(push, 0)
	  #include <Windows.h>
	  #pragma warning(pop)
#endif



#if defined(_MSC_VER)
	#pragma warning(disable: 4996) // This function or variable may be unsafe / deprecated.
#endif


using namespace EA::Thread;


typedef intptr_t (*FutexTestThreadFunction)(void*);

const int kMaxConcurrentThreadCount = EATHREAD_MAX_CONCURRENT_THREAD_COUNT;


///////////////////////////////////////////////////////////////////////////////
// FWorkData
//
struct FWorkData
{
	volatile bool mbShouldQuit;
	Futex         mFutex;
	char          mUnused[7000];      // We intentionally put a big buffer between these.
	int           mThreadWithLock[kMaxConcurrentThreadCount];
	AtomicInt32   mThreadCount;
	AtomicInt32   mnErrorCount;

	FWorkData() : mbShouldQuit(false), mFutex(), mThreadCount(0), mnErrorCount(0) { memset(mThreadWithLock, 0, sizeof(mThreadWithLock)); }

private:
	// Prevent default generation of these functions by not defining them
	FWorkData(const FWorkData& rhs);               // copy constructor
	FWorkData& operator=(const FWorkData& rhs);    // assignment operator
};




///////////////////////////////////////////////////////////////////////////////
// TestThreadFutexSingle
//
static int TestThreadFutexSingle()
{

	// Formerly declared as a global because VC++ for XBox 360 was found to be possibly throwing 
	// away the atomic operations when it found that the futex was local to the function.
	
	// Now that Xbox 360 support has been removed, the futex has been tentatively moved back to
	// local scope. Of course, if this problem manifests again, the change will be reversed.
	Futex futexSingle;

	int nErrorCount(0);

	EA::UnitTest::ReportVerbosity(1, "\nSimple test...\n");

	// Single-threaded tests
	int nLockCount;

	EATEST_VERIFY_MSG(!futexSingle.HasLock(), "Futex failure.");

	nLockCount = futexSingle.GetLockCount();
	EATEST_VERIFY_MSG(nLockCount == 0, "Futex failure.");

	futexSingle.Lock();
	nLockCount = futexSingle.GetLockCount();
	EATEST_VERIFY_MSG(nLockCount == 1, "Futex failure.");

	EATEST_VERIFY_MSG(futexSingle.HasLock(), "Futex failure.");

	futexSingle.Lock();
	nLockCount = futexSingle.GetLockCount();
	EATEST_VERIFY_MSG(nLockCount == 2, "Futex failure.");

	futexSingle.Unlock();
	nLockCount = futexSingle.GetLockCount();
	EATEST_VERIFY_MSG(nLockCount == 1, "Futex failure.");

	futexSingle.Unlock();
	nLockCount = futexSingle.GetLockCount();
	EATEST_VERIFY_MSG(nLockCount == 0, "Futex failure.");

	futexSingle.Lock();
	nLockCount = futexSingle.GetLockCount();
	EATEST_VERIFY_MSG(nLockCount == 1, "Futex failure.");

	futexSingle.Unlock();
	nLockCount = futexSingle.GetLockCount();
	EATEST_VERIFY_MSG(nLockCount ==  0, "Futex failure.");

	EATEST_VERIFY_MSG(!futexSingle.HasLock(), "Futex failure.");

	return nErrorCount;
}




///////////////////////////////////////////////////////////////////////////////
// FutexTestThreadFunction1
//
static intptr_t FutexTestThreadFunction1(void* pvWorkData)
{
	int           nErrorCount  = 0;
	FWorkData*    pWorkData    = (FWorkData*)pvWorkData;
	const int32_t nThreadIndex = pWorkData->mThreadCount++;
	ThreadId      threadId     = GetThreadId();

	EA::UnitTest::ReportVerbosity(1, "FutexTestThreadFunction1 created: %s, thread index %d\n", EAThreadThreadIdToString(threadId), nThreadIndex);

	while(!pWorkData->mbShouldQuit)
	{
		int nRecursiveLockCount(rand() % 3);
		int i;

		for(i = 0; i < nRecursiveLockCount; ++i)
		{
			pWorkData->mFutex.Lock();
			pWorkData->mThreadWithLock[nThreadIndex]++;

			for(int j = 0; j < kMaxConcurrentThreadCount; ++j)
			{
				// Make sure this thread has the lock.
				EATEST_VERIFY_F((j == nThreadIndex) || !pWorkData->mThreadWithLock[j], "Futex failure (thread %s)\n", EAThreadThreadIdToString(threadId));
			}

			ThreadCooperativeYield(); // Used by cooperative threading platforms.
		}

		for(; nRecursiveLockCount > 0; --nRecursiveLockCount)
		{
			pWorkData->mThreadWithLock[nThreadIndex]--;

			// Verify that N locks are set.
			int nLockCount = pWorkData->mFutex.GetLockCount();

			EATEST_VERIFY_MSG(nLockCount == nRecursiveLockCount, "Futex failure.");

			if(nLockCount != nRecursiveLockCount)
				pWorkData->mbShouldQuit = true;
			
			// Verify the unlock result.
			pWorkData->mFutex.Unlock();
			nLockCount = pWorkData->mFutex.GetLockCount();

			EATEST_VERIFY_MSG((nRecursiveLockCount == 1) || (nLockCount != nRecursiveLockCount), "Futex failure.");

			if(nRecursiveLockCount > 1) // If we have remaining locks...
			{
				for(int j = 0; j < kMaxConcurrentThreadCount; ++j)
				{
					// Make sure this thread has the lock.
					EATEST_VERIFY_F((j == nThreadIndex) || !pWorkData->mThreadWithLock[j], "Futex failure (thread %s)\n", EAThreadThreadIdToString(threadId));
				}
			}
		
			ThreadCooperativeYield(); // Used by cooperative threading platforms.
		}

		if(nErrorCount)
			pWorkData->mbShouldQuit = true;
		else
			EA::UnitTest::ThreadSleepRandom(100, 200);
	}

	pWorkData->mnErrorCount += nErrorCount;

	return 0;
}


///////////////////////////////////////////////////////////////////////////////
// FutexTestThreadFunction2
//
// In this function we test Lock, attempting to detect memory synchronization
// problems that could occur with an incorrect Futex implementation.
//
static intptr_t FutexTestThreadFunction2(void* pvWorkData)
{
	int           nErrorCount  = 0;
	FWorkData*    pWorkData    = (FWorkData*)pvWorkData;
	const int32_t nThreadIndex = pWorkData->mThreadCount++;
	ThreadId      threadId     = GetThreadId();

	EA::UnitTest::ReportVerbosity(1, "FutexTestThreadFunction2 created: %s, thread index %d\n", EAThreadThreadIdToString(threadId), nThreadIndex);

	while(!pWorkData->mbShouldQuit)
	{
		pWorkData->mFutex.Lock();
		pWorkData->mThreadWithLock[nThreadIndex] = 1;

		for(int j = 0; j < kMaxConcurrentThreadCount; ++j)
		{
			// Make sure this thread has the lock.
			EATEST_VERIFY_F((j == nThreadIndex) || !pWorkData->mThreadWithLock[j], "Futex failure (thread %s)\n", EAThreadThreadIdToString(threadId));
		}

		pWorkData->mThreadWithLock[nThreadIndex] = 0;
		pWorkData->mFutex.Unlock();
		ThreadCooperativeYield(); // Used by cooperative threading platforms.

		if(nErrorCount)
			pWorkData->mbShouldQuit = true;
	}

	pWorkData->mnErrorCount += nErrorCount;

	return 0;
}


///////////////////////////////////////////////////////////////////////////////
// FutexTestThreadFunction3
//
// In this function we test TryLock.
//
static intptr_t FutexTestThreadFunction3(void* pvWorkData)
{
	int           nErrorCount  = 0;
	FWorkData*    pWorkData    = (FWorkData*)pvWorkData;
	const int32_t nThreadIndex = pWorkData->mThreadCount++;
	ThreadId      threadId     = GetThreadId();

	EA::UnitTest::ReportVerbosity(1, "FutexTestThreadFunction3 created: %s, thread index %d\n", EAThreadThreadIdToString(threadId), nThreadIndex);

	for(int i = 0; !pWorkData->mbShouldQuit; ++i)
	{
		// We make sure to mix TryLock usage with Lock usage.
		bool bResult;

		if(((i % 4) != 0)) // 3/4 of the time, use TryLock, 1/4 of the time, use Lock.
			bResult = pWorkData->mFutex.TryLock();
		else
		{
			pWorkData->mFutex.Lock();
			bResult = true;
		}

		if(bResult)
		{
			pWorkData->mThreadWithLock[nThreadIndex] = 1;

			for(int j = 0; j < kMaxConcurrentThreadCount; ++j)
			{
				// Make sure this thread has the lock.
				EATEST_VERIFY_F((j == nThreadIndex) || !pWorkData->mThreadWithLock[j], "Futex failure (thread %s)\n", EAThreadThreadIdToString(threadId));
			}

			pWorkData->mThreadWithLock[nThreadIndex] = 0;
			pWorkData->mFutex.Unlock();

			if(nErrorCount)
				pWorkData->mbShouldQuit = true;
		}

		ThreadCooperativeYield(); // Used by cooperative threading platforms.
	}

	pWorkData->mnErrorCount += nErrorCount;

	return 0;
}


///////////////////////////////////////////////////////////////////////////////
// TestThreadFutexMulti
//
static int TestThreadFutexMulti(FutexTestThreadFunction pFutexTestThreadFunction, int nThreadCount)
{
	int              nErrorCount(0);
	FWorkData* const pWorkData = new FWorkData; 
	Thread           thread[kMaxConcurrentThreadCount];
	Thread::Status   status;
	int              i;

	EA::UnitTest::ReportVerbosity(1, "Multithreaded test...\n");

	if(nThreadCount > kMaxConcurrentThreadCount)
		nThreadCount = kMaxConcurrentThreadCount;

	for(i = 0; i < nThreadCount; i++)
		thread[i].Begin(pFutexTestThreadFunction, pWorkData);

	EA::UnitTest::ThreadSleepRandom(gTestLengthSeconds * 1000, gTestLengthSeconds * 1000);

	pWorkData->mbShouldQuit = true;

	for(i = 0; i < nThreadCount; i++)
	{
		status = thread[i].WaitForEnd(GetThreadTime() + 30000);
		EATEST_VERIFY_MSG(status == EA::Thread::Thread::kStatusEnded, "Futex/Thread failure: Thread(s) didn't end.");
	}

	nErrorCount += (int)pWorkData->mnErrorCount;

	delete pWorkData;

	return nErrorCount;
}


///////////////////////////////////////////////////////////////////////////////
// TestThreadFutexSpeed
//
static int TestThreadFutexSpeed()
{
	int          nErrorCount = 0;
	size_t       i;
	uint64_t     t0, t1, tDelta;
	const size_t kLoopCount = 1000000;

	EA::UnitTest::ReportVerbosity(1, "\nSpeed test...\n");

	//////////////////////////////////////////////////
	// Futex
	{
		Futex f;

		t0 = EA::StdC::Stopwatch::GetCPUCycle();
		f.Lock();
		f.Unlock();
		t1 = EA::StdC::Stopwatch::GetCPUCycle();

		t0 = EA::StdC::Stopwatch::GetCPUCycle();

		for(i = 0; i < kLoopCount; i++)
		{
			f.Lock();
			f.Unlock();
		}

		t1     = EA::StdC::Stopwatch::GetCPUCycle();
		tDelta = t1 - t0;

		EA::UnitTest::ReportVerbosity(1, "Futex time (ticks): %" PRIu64 "\n", tDelta);
	}
	//////////////////////////////////////////////////


	//////////////////////////////////////////////////
	// Mutex
	{
		EA::Thread::Mutex m;

		t0 = EA::StdC::Stopwatch::GetCPUCycle();
		m.Lock();
		m.Unlock();
		t1 = EA::StdC::Stopwatch::GetCPUCycle();

		t0 = EA::StdC::Stopwatch::GetCPUCycle();

		for(i = 0; i < kLoopCount; i++)
		{
			m.Lock();
			m.Unlock();
		}

		t1     = EA::StdC::Stopwatch::GetCPUCycle();
		tDelta = t1 - t0;

		EA::UnitTest::ReportVerbosity(1, "Mutex time (ticks): %" PRIu64 "\n", tDelta);
	}
	//////////////////////////////////////////////////


	#if defined(EA_PLATFORM_MICROSOFT) && !defined(EA_PLATFORM_WINDOWS_PHONE) && !(defined(EA_PLATFORM_WINDOWS) && !EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP))

		//////////////////////////////////////////////////
		// HMUTEX
		{
			HANDLE hMutex = CreateMutexA(NULL, false, NULL);

			if (hMutex != NULL)
			{
				WaitForSingleObject(hMutex, INFINITE);
				ReleaseMutex(hMutex);

				t0 = EA::StdC::Stopwatch::GetCPUCycle();

				for(i = 0; i < kLoopCount; i++)
				{
					WaitForSingleObject(hMutex, INFINITE);
					ReleaseMutex(hMutex);
				}

				t1     = EA::StdC::Stopwatch::GetCPUCycle();
				tDelta = t1 - t0;

				CloseHandle(hMutex);

				EA::UnitTest::ReportVerbosity(1, "Windows HMUTEX time (ticks): %" PRIu64 "\n", tDelta);
			}
		}
		//////////////////////////////////////////////////

		//////////////////////////////////////////////////
		// Critical section
		{
			CRITICAL_SECTION cs;
			InitializeCriticalSection(&cs);

			EnterCriticalSection(&cs);
			LeaveCriticalSection(&cs);

			t0 = EA::StdC::Stopwatch::GetCPUCycle();

			for(i = 0; i < kLoopCount; i++)
			{
				EnterCriticalSection(&cs);
				LeaveCriticalSection(&cs);
			}

			t1     = EA::StdC::Stopwatch::GetCPUCycle();
			tDelta = t1 - t0;

			DeleteCriticalSection(&cs);

			EA::UnitTest::ReportVerbosity(1, "Windows CriticalSection time (ticks): %" PRIu64 "\n", tDelta);
		}
		//////////////////////////////////////////////////


	#endif

	return nErrorCount;
}


static int TestThreadFutexRegressions()
{
	int nErrorCount(0);

	#if EA_THREADS_AVAILABLE && EATHREAD_DEBUG_FUTEX_HANG_ENABLED
	{
		// Test the ability of Futex to report the callstack of another thread holding a futex.
		struct FutexCallstackTestThread : public EA::Thread::IRunnable
		{
			ThreadParameters    mThreadParams;  // 
			EA::Thread::Thread  mThread;        // The Thread object.
			EA::Thread::Futex*  mpFutex;        // A Futex.

			FutexCallstackTestThread() : mThreadParams(), mThread(), mpFutex(NULL) {}
			FutexCallstackTestThread(const FutexCallstackTestThread&){} // Avoid compiler warnings.
			void operator=(const FutexCallstackTestThread&){}           // Avoid compiler warnings.

			intptr_t Run(void*)
			{
				if(EA::StdC::Strstr(mThreadParams.mpName, "0")) // If we are thread 0...
				{
					mpFutex->Lock();
					mpFutex->Lock();
					EA::Thread::ThreadSleep(4000);
					mpFutex->Unlock();
					mpFutex->Unlock();
				}
				else
				{
					mpFutex->Lock();  // This should result in a printf tracing two thread 0 lock callstacks.
					mpFutex->Unlock();
				}
				return 0;
			}
		};

		FutexCallstackTestThread thread[2];
		EA::Thread::Futex futex;

		for(int i = 0; i < 3; i++)
		{
			thread[0].mThreadParams.mpName = "FutexTest0";
			thread[0].mpFutex = &futex;
			thread[0].mThread.Begin(&thread[0], NULL, &thread[0].mThreadParams);

			EA::UnitTest::ThreadSleep(300);

			thread[1].mThreadParams.mpName = "FutexTest1";
			thread[1].mpFutex = &futex;
			thread[1].mThread.Begin(&thread[1], NULL, &thread[1].mThreadParams);

			EA::UnitTest::ThreadSleep(5000);

			for(int i = 0; i < 2; i++)
				thread[i].mThread.WaitForEnd();
		}
	}
	#endif

	return nErrorCount;
}



#define EATHREAD_DEBUG_FUTEX_HAMMER_ENABLED 1
// Check multithreaded correctness of Futex.
// Here we hammer on the futex via multiple threads while incrementing non atomic counter
// if the lock ever fails the global count will be incorrect.

volatile uint32_t  gCommonCount = 0;

static int TestThreadFutexHammer()
{
	int nErrorCount(0);

	#if EA_THREADS_AVAILABLE && EATHREAD_DEBUG_FUTEX_HAMMER_ENABLED
	{
		static const int NUM_SPINNING_THREADS = 4;
		static const uint32_t MAX_NUM_LOOPS = 1 << 5;

		// Test the ability of Futex to report the callstack of another thread holding a futex.
		struct FutexCallstackTestThread : public EA::Thread::IRunnable
		{
			ThreadParameters    mThreadParams;  // 
			EA::Thread::Thread  mThread;        // The Thread object.
			EA::Thread::Futex*  mpFutex;        // A Futex.
			uint32_t  mThreadLocalId;

			FutexCallstackTestThread() : mThreadParams(), mThread(), mpFutex(NULL) {}
			FutexCallstackTestThread(const FutexCallstackTestThread&) {} // Avoid compiler warnings.
			void operator=(const FutexCallstackTestThread&) {}           // Avoid compiler warnings.

			intptr_t Run(void*)
			{
				for (uint32_t i = 0; i < MAX_NUM_LOOPS; i++) 
				{
					mpFutex->Lock();
					gCommonCount++;
					mpFutex->Unlock();
				}
				return 0;
			}
		};


		FutexCallstackTestThread thread[NUM_SPINNING_THREADS];
		EA::Thread::Futex futex;
		gCommonCount = 0; // for multiple runs

		for(int i = 0; i < NUM_SPINNING_THREADS; i++)
		{
			thread[i].mpFutex = &futex;
			thread[i].mThread.Begin(&thread[i], NULL, &thread[i].mThreadParams);
		}

		EA::UnitTest::ThreadSleep(13);
		for(int i = 0; i < NUM_SPINNING_THREADS; i++)
			thread[i].mThread.WaitForEnd();

		EATEST_VERIFY_MSG(gCommonCount == NUM_SPINNING_THREADS * MAX_NUM_LOOPS, "Multithreaded futex test failed, non atomic counter is incorrect");
	}
	#endif

	return nErrorCount;
}




///////////////////////////////////////////////////////////////////////////////
// TestThreadFutex
//
int TestThreadFutex()
{
	int nErrorCount(0);

	nErrorCount += TestThreadFutexSingle();
	nErrorCount += TestThreadFutexSpeed();
	nErrorCount += TestThreadFutexRegressions();

	// hammer on the futex a few times
	for(int j=0; j <1;j++)
	{
		nErrorCount +=  TestThreadFutexHammer();
	}

	#if EA_THREADS_AVAILABLE
		nErrorCount += TestThreadFutexMulti(FutexTestThreadFunction1, 4);
		nErrorCount += TestThreadFutexMulti(FutexTestThreadFunction2, 4);
		nErrorCount += TestThreadFutexMulti(FutexTestThreadFunction3, 4);
	#endif

	return nErrorCount;
}





