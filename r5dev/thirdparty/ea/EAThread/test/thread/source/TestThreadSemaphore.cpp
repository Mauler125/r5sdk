///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "TestThread.h"
#include <EATest/EATest.h>
#include <EAStdC/EAStopwatch.h>
#include <eathread/eathread.h>
#include <eathread/eathread_semaphore.h>
#include <eathread/eathread_thread.h>
#include <eathread/eathread_atomic.h>


using namespace EA::Thread;


const int kMaxConcurrentThreadCount = EATHREAD_MAX_CONCURRENT_THREAD_COUNT;


struct SWorkData
{
	volatile bool   mbShouldQuit;
	Semaphore       mSemaphore;
	int             mnPostCount;        // The count to use for Semaphore::Post()
	AtomicInt32     mnExpectedCount;
	AtomicInt32     mnThreadCount;
	AtomicInt32     mnErrorCount;

	SWorkData(const SemaphoreParameters& sp)
	  : mbShouldQuit(false), mSemaphore(&sp, false), 
		mnPostCount(1), mnExpectedCount(0), mnThreadCount(0), 
		mnErrorCount(0) {}

	// define copy ctor and assignment operator
	// so the compiler does define them intrisically
	SWorkData(const SWorkData& rhs);                  // copy constructor
	SWorkData& operator=(const SWorkData& rhs);    // assignment operator
};



static intptr_t SemaphoreTestThreadFunction(void* pvWorkData)
{
	SWorkData* pWorkData   = (SWorkData*)pvWorkData;
	int        nErrorCount = 0;
	ThreadId   threadId    = GetThreadId();

	EA::UnitTest::ReportVerbosity(1, "Semaphore test function created: %s\n", EAThreadThreadIdToString(threadId));

	const AtomicInt32::ValueType nThreadCount = pWorkData->mnThreadCount++; // AtomicInt32 operation.
	const AtomicInt32::ValueType nPostCount   = pWorkData->mnPostCount;

	#if defined(EA_PLATFORM_DESKTOP)    // If the platform tends to run on fast hardware...
		const unsigned kShortSleepMin = 50;
		const unsigned kShortSleepMax = 100;
	#else
		const unsigned kShortSleepMin = 100;
		const unsigned kShortSleepMax = 200;
	#endif

	while(!pWorkData->mbShouldQuit)
	{
		if((nThreadCount % 2) == 0) // If the first thread or the third thread...
		{
			// Create 'work'.
			pWorkData->mnExpectedCount += nPostCount; // Atomic operation.
			pWorkData->mSemaphore.Post(nPostCount);
			EA::UnitTest::ThreadSleepRandom(kShortSleepMin, kShortSleepMax);

			// Get the amount of 'work' that is expected.
			int nWaitCount = nPostCount;

			// Wait for the 'work' to be queued and signalled.
			while(nWaitCount-- > 0)
			{
				const int result = pWorkData->mSemaphore.Wait(GetThreadTime() + 2000);

				EATEST_VERIFY_MSG((result >= 0) || (result == Semaphore::kResultTimeout), "Semaphore failure 4a: semaphore.Wait()\n");

				if(result >= 0) // If we we able to acquire the semaphore...
					--pWorkData->mnExpectedCount; // Atomic operation.
				// else timeout occurred. Every time we have this timeout we miss decrementing the semaphore by one, and thus the expected semaphore count rises by one. It doesn't mean there's a bug, it just means the expected count migrates higher as we get timeouts.

				EA::UnitTest::ThreadSleepRandom(kShortSleepMin, kShortSleepMax);
			}
		}
		else
		{
			// Get the amount of 'work' that is expected.
			int nWaitCount = nPostCount;

			// Wait for the 'work' to be queued and signalled.
			while(nWaitCount-- > 0)
			{
				const int result = pWorkData->mSemaphore.Wait(GetThreadTime() + 2000);

				EATEST_VERIFY_MSG((result >= 0) || (result == Semaphore::kResultTimeout), "Semaphore failure 4b: semaphore.Wait()\n");

				if(result >= 0) // If we we able to acquire the semaphore...
					--pWorkData->mnExpectedCount; // Atomic operation.
				// else timeout occurred. Every time we have this timeout we miss decrementing the semaphore by one, and thus the expected semaphore count rises by one. It doesn't mean there's a bug, it just means the expected count migrates higher as we get timeouts.

				EA::UnitTest::ThreadSleepRandom(kShortSleepMin, kShortSleepMax);
			}

			// Create 'work'.
			pWorkData->mnExpectedCount += nPostCount; // Atomic operation.
			pWorkData->mSemaphore.Post(nPostCount);
			EA::UnitTest::ThreadSleepRandom(kShortSleepMin, kShortSleepMax);
		}

		EATEST_VERIFY_MSG(pWorkData->mSemaphore.GetCount() >= 0, "Semaphore failure 4c: The count should always be >= 0.\n");

		// We restrict this assert to desktop platforms because the expected value migrates upward on every Wait timeout, and on slower platforms there could be a lot of such timeouts.
		#if defined(EA_PLATFORM_DESKTOP)
			EATEST_VERIFY_MSG(pWorkData->mSemaphore.GetCount() < 200, "Semaphore failure 4d: The count should always be a small value.\n");
		#endif
	}

	pWorkData->mnErrorCount = nErrorCount;

	return 0;
}



struct BadSignalTestData
{
	 EA::Thread::Semaphore  mSemaphore;             // The Semaphore that we are using.
	 AtomicInt32            mnOKWaitCount;          // The number of Wait timeouts that occurred.
	 AtomicInt32            mnTimeoutWaitCount;     // The number of Wait OK returns that occurred.
	 AtomicInt32            mnErrorWaitCount;       // The number of Wait error returns that occurred.
	 int                    mnWaitTimeout;          // How long a waiter waits before timing out.
	 int                    mnWaitThreadCount;      // How many waiter threads there are. Caller sets up this value.
	 int                    mnPostThreadCount;      // How many poster threads there are. Caller sets up this value.
	 int                    mnPostCount;            // How many signals the poster should Post. Needs to be less than mnWaitThreadCount for this test. Caller sets up this value.
	 int                    mnPostDelay;            // How long the poster waits before Posting. Needs to be significantly less than mnWaitTimeout.

	 BadSignalTestData()
	  : mSemaphore(), mnOKWaitCount(0), mnTimeoutWaitCount(0), mnErrorWaitCount(0),
		mnWaitTimeout(0), mnWaitThreadCount(0), mnPostThreadCount(0),
		mnPostCount(0), mnPostDelay(0) { }

	// Define copy ctor and assignment operator
	// so the compiler does define them intrisically
	BadSignalTestData(const BadSignalTestData& rhs);               // copy constructor
	BadSignalTestData& operator=(const BadSignalTestData& rhs);    // assignment operator
};


static intptr_t BadSignalTestWaitFunction(void *data)
{
	BadSignalTestData* const pBSTD = (BadSignalTestData*)data;

	const int waitResult = pBSTD->mSemaphore.Wait(EA::Thread::GetThreadTime() + pBSTD->mnWaitTimeout);

	if(waitResult == Semaphore::kResultTimeout)
		pBSTD->mnTimeoutWaitCount.Increment();
	else if(waitResult >= 0)
		pBSTD->mnOKWaitCount.Increment();
	else
		pBSTD->mnErrorWaitCount.Increment();

	return 0;
}


static intptr_t BadSignalTestPostFunction(void *data)
{
	BadSignalTestData* const pBSTD = (BadSignalTestData*)data;

	ThreadSleep((ThreadTime) pBSTD->mnPostDelay);
	pBSTD->mSemaphore.Post(pBSTD->mnPostCount); // Intentionally post less than the number of waiting threads.

	return 0;
}




int TestThreadSemaphore()
{
	int nErrorCount(0);

	{ // Single threaded test.
		const int kInitialCount(4);
		int nResult;

		SemaphoreParameters sp(kInitialCount, false, "SingleThreadTest");
		Semaphore semaphore(&sp);

		nResult = semaphore.GetCount();
		EATEST_VERIFY_F(nResult == kInitialCount, "Semaphore failure 2a: semaphore.GetCount(). result = %d\n", nResult);

		// This triggers an assert and so cannot be tested here:
		// bResult = semaphore.SetCount(10);
		// EATEST_VERIFY_MSG(!bResult, "Semaphore failure in semaphore.SetCount(10)\n");

		// Grab the full count, leaving none.
		for(int i(0); i < kInitialCount; i++)
		{
			nResult = semaphore.Wait(kTimeoutNone);
			EATEST_VERIFY_F(nResult == kInitialCount - i - 1, "Semaphore failure 2b: semaphore.Wait(kTimeoutNone). result = %d\n", nResult);

			nResult = semaphore.GetCount();
			EATEST_VERIFY_F(nResult == kInitialCount - i - 1, "Semaphore failure 2c: semaphore.GetCount(). result = %d\n", nResult);
		}

		nResult = semaphore.Wait(kTimeoutImmediate);
		EATEST_VERIFY_F(nResult == Semaphore::kResultTimeout, "Semaphore failure 2d: semaphore.Wait(kTimeoutImmediate). result = %d\n", nResult);

		nResult = semaphore.GetCount();
		EATEST_VERIFY_F(nResult == 0, "Semaphore failure 2e: semaphore.GetCount(). result = %d\n", nResult);

		nResult = semaphore.Post(2);
		EATEST_VERIFY_F(nResult == 2, "Semaphore failure 2f: semaphore.Post(2). result = %d\n", nResult);

		nResult = semaphore.Post(2);
		EATEST_VERIFY_F(nResult == 4, "Semaphore failure 2g: semaphore.Post(2). result = %d\n", nResult);
	}

	
	#if EA_THREADS_AVAILABLE

		{   // Bad signal test.
			BadSignalTestData bstd;
			Thread            thread[4];

			// These values need to be set up right or else this test won't work. 
			// What we are trying to do here is make certain that N number of Semaphore
			// waiters time out, while M waiters succeed.
			bstd.mnWaitTimeout     = 5000;
			bstd.mnPostDelay       = 2000;
			bstd.mnWaitThreadCount = 3;      // mnWaitThreadCount + mnPostThreadCount should be [4].
			bstd.mnPostThreadCount = 1;
			bstd.mnPostCount       = 1;

			thread[0].Begin(BadSignalTestWaitFunction, &bstd);
			thread[1].Begin(BadSignalTestWaitFunction, &bstd);
			thread[2].Begin(BadSignalTestWaitFunction, &bstd);
			thread[3].Begin(BadSignalTestPostFunction, &bstd);

			// Wait for the threads to be completed
			thread[0].WaitForEnd();
			thread[1].WaitForEnd();
			thread[2].WaitForEnd();
			thread[3].WaitForEnd();

			EATEST_VERIFY_MSG(bstd.mnTimeoutWaitCount == (bstd.mnWaitThreadCount - (bstd.mnPostThreadCount * bstd.mnPostCount)), "Semaphore failure 1a: bad signal test.\n");
			EATEST_VERIFY_MSG(bstd.mnErrorWaitCount == 0, "Semaphore failure 1b: bad signal test.\n");
			EATEST_VERIFY_MSG(bstd.mnOKWaitCount == (bstd.mnPostThreadCount * bstd.mnPostCount), "Semaphore failure 1c: bad signal test.\n");
		}

		{  // Multithreaded test

			// Problem: In the case of the inter-process test below, since we are using a named semaphore it's possible that
			// if two instances of this unit test are run on the same machine simultaneously, they will collide because
			// they are using the same semaphore. This applies only to true multi-process-capable machines.
			uint64_t            cycle64 = EA::StdC::Stopwatch::GetCPUCycle();
			uint16_t            cycle16 = (uint16_t)((uint16_t)(cycle64 >> 48) ^ (uint16_t)(cycle64 >> 32) ^ (uint16_t)(cycle64 >> 16) ^ (uint16_t)(cycle64 >> 0)); // Munge all the bits together because some platforms might have zeroed low bits; others high bits. This will work for all.
			char                name[16]; sprintf(name, "SMT%u", (unsigned)cycle16);
			SemaphoreParameters semaphoreParameters(0, false, name);
			#if defined(EA_PLATFORM_DESKTOP)
			const int           kLoopCount = 16;
			#else
			const int           kLoopCount = 4;
			#endif

			for(int j = 1; j <= kLoopCount; j++) // Intentionally start with 1, as we use j below.
			{
				if(semaphoreParameters.mbIntraProcess)
					semaphoreParameters.mbIntraProcess = false;
				else
					semaphoreParameters.mbIntraProcess = true;

				SWorkData workData(semaphoreParameters); 
				workData.mnPostCount = (j % 4);

				const int      kThreadCount(kMaxConcurrentThreadCount - 1);
				Thread         thread[kThreadCount];
				ThreadId       threadId[kThreadCount];
				Thread::Status status;
				int i;

				for(i = 0; i < kThreadCount; i++)
				{
					threadId[i] = thread[i].Begin(SemaphoreTestThreadFunction, &workData);

					EATEST_VERIFY_MSG(threadId[i] != kThreadIdInvalid, "Semaphore failure 3a: Couldn't create thread.\n");
				}

				EA::UnitTest::ThreadSleepRandom(gTestLengthSeconds*2000, gTestLengthSeconds*2000);

				workData.mbShouldQuit = true;

				#if defined(EA_PLATFORM_DESKTOP)
					EA::UnitTest::ThreadSleepRandom(500, 500);
				#else
					EA::UnitTest::ThreadSleepRandom(1000, 1000);
				#endif

				// We seed the semaphore with more posts because timing issues could cause the 
				// worker threads to get hung waiting for posts but it really isn't because
				// of a problem with the semaphore. By the time all threads have quit, the 
				// expected count should be equal to the increment.
				const int kIncrement = 20;
				workData.mnExpectedCount += kIncrement; // AtomicInt32 operation
				workData.mSemaphore.Post(kIncrement);    

				#if defined(EA_PLATFORM_DESKTOP)
					EA::UnitTest::ThreadSleepRandom(1000, 1000);
				#else
					EA::UnitTest::ThreadSleepRandom(2000, 2000);
				#endif

				for(i = 0; i < kThreadCount; i++)
				{
					if(threadId[i] != kThreadIdInvalid)
					{
						status = thread[i].WaitForEnd(GetThreadTime() + 30000);
						EATEST_VERIFY_MSG(status == Thread::kStatusEnded, "Semaphore failure 3b: Thread(s) didn't end.\n");
					}
				}

				// Normally the Semaphore::GetCount function returns a value that is volatile, but we know that 
				// there aren't any threads using mSemaphore any more, so GetCount returns a value we can rely on.
				EATEST_VERIFY_MSG(workData.mnExpectedCount == workData.mSemaphore.GetCount(), "Semaphore failure 3c: Unexpected value.\n");
				if(workData.mnExpectedCount != workData.mSemaphore.GetCount())
				{
					EA::UnitTest::ReportVerbosity(1, "    Thread count: %d, Intraprocess: %s, Post count: %d, Expected count: %d, Semaphore GetCount: %d\n", 
												  kThreadCount, semaphoreParameters.mbIntraProcess ? "yes" : "no", workData.mnPostCount, (int)workData.mnExpectedCount.GetValue(), workData.mSemaphore.GetCount());
				}

				nErrorCount += (int)workData.mnErrorCount;
			}

		}

	#endif // EA_THREADS_AVAILABLE

	return nErrorCount;
}











