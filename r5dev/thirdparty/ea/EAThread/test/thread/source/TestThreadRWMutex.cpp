///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "TestThread.h"
#include <EATest/EATest.h>
#include <eathread/eathread_rwmutex.h>
#include <eathread/eathread_thread.h>
#include <stdlib.h>


using namespace EA::Thread;


const int kMaxConcurrentThreadCount = EATHREAD_MAX_CONCURRENT_THREAD_COUNT;


struct RWMWorkData
{
	volatile bool   mbShouldQuit;
	RWMutex         mRWMutex;
	volatile int    mnExpectedValue;
	volatile int    mnCalculatedValue;
	AtomicInt32     mnErrorCount;

	RWMWorkData() : mbShouldQuit(false), mRWMutex(NULL, true), mnExpectedValue(0), 
					mnCalculatedValue(0), mnErrorCount(0) {}

	// define copy ctor and assignment operator
	// so the compiler does define them intrisically
	RWMWorkData(const RWMWorkData& rhs);               // copy constructor
	RWMWorkData& operator=(const RWMWorkData& rhs);    // assignment operator
};


static intptr_t ReaderFunction(void* pvWorkData)
{
	int          nErrorCount = 0;
	RWMWorkData* pWorkData   = (RWMWorkData*)pvWorkData;
	ThreadId     threadId    = GetThreadId();

	EA::UnitTest::ReportVerbosity(1, "RWMutex reader test function created: %s\n", EAThreadThreadIdToString(threadId));

	while(!pWorkData->mbShouldQuit)
	{
		const int nRecursiveLockCount(rand() % 3);
		int i, nLockResult, nLocks = 0;

		for(i = 0; i < nRecursiveLockCount; i++)
		{
			// Do a lock but allow for the possibility of occasional timeout.
			nLockResult = pWorkData->mRWMutex.Lock(RWMutex::kLockTypeRead, GetThreadTime() + 20);

			EATEST_VERIFY_MSG(nLockResult != RWMutex::kResultError, "RWMutex failure");

			if(nLockResult > 0)
			{
				nLocks++;

				EA::UnitTest::ReportVerbosity(2, "CValue = %d; EValue = %d\n", pWorkData->mnCalculatedValue, pWorkData->mnExpectedValue);
				EATEST_VERIFY_MSG(pWorkData->mnCalculatedValue == pWorkData->mnExpectedValue, "RWMutex failure");
			}
			ThreadCooperativeYield(); // Used by cooperative threading platforms.
		}

		while(nLocks > 0)
		{
			// Verify no write locks are set.
			nLockResult = pWorkData->mRWMutex.GetLockCount(RWMutex::kLockTypeWrite);
			EATEST_VERIFY_MSG(nLockResult == 0, "RWMutex failure");

			// Verify at least N read locks are set.
			nLockResult = pWorkData->mRWMutex.GetLockCount(RWMutex::kLockTypeRead);
			EATEST_VERIFY_MSG(nLockResult >= nLocks, "RWMutex failure");

			// Verify there is one less read lock set.
			nLockResult = pWorkData->mRWMutex.Unlock();
			EATEST_VERIFY_MSG(nLockResult >= nLocks-1, "RWMutex failure");

			nLocks--;
			ThreadCooperativeYield(); // Used by cooperative threading platforms.
		}

		EA::UnitTest::ThreadSleepRandom(100, 200);
	}

	pWorkData->mnErrorCount += nErrorCount;

	return 0;
}


static intptr_t WriterFunction(void* pvWorkData)
{
	int          nErrorCount = 0;
	RWMWorkData* pWorkData   = (RWMWorkData*)pvWorkData;
	ThreadId     threadId    = GetThreadId();

	EA::UnitTest::ReportVerbosity(1, "RWMutex writer test function created: %s\n", EAThreadThreadIdToString(threadId));

	while(!pWorkData->mbShouldQuit)
	{
		// Do a lock but allow for the possibility of occasional timeout.
		int nLockResult = pWorkData->mRWMutex.Lock(RWMutex::kLockTypeWrite, GetThreadTime() + 30);
		EATEST_VERIFY_MSG(nLockResult != RWMutex::kResultError, "RWMutex failure");

		ThreadCooperativeYield(); // Used by cooperative threading platforms.

		if(nLockResult > 0)
		{
			// Verify exactly one write lock is set.
			nLockResult = pWorkData->mRWMutex.GetLockCount(RWMutex::kLockTypeWrite);
			EATEST_VERIFY_MSG(nLockResult == 1, "RWMutex failure");

			// What we do here is spend some time manipulating mnExpectedValue and mnCalculatedValue
			// while we have the write lock. We change their values in a predicable way but before 
			// we are done mnCalculatedValue has been incremented by one and both values are equal.
			const uintptr_t x = (uintptr_t)pWorkData;

			pWorkData->mnExpectedValue     = -1;
			EA::UnitTest::ThreadSleepRandom(10, 20);
			pWorkData->mnCalculatedValue *= 50;
			EA::UnitTest::ThreadSleepRandom(10, 20);
			pWorkData->mnCalculatedValue /= (int)(((x + 1) / x) * 50); // This will always be the same as simply '/= 50'.
			EA::UnitTest::ThreadSleepRandom(10, 20);
			pWorkData->mnCalculatedValue += 1;
			EA::UnitTest::ThreadSleepRandom(10, 20);
			pWorkData->mnExpectedValue     = pWorkData->mnCalculatedValue;

			// Verify no read locks are set.
			nLockResult = pWorkData->mRWMutex.GetLockCount(RWMutex::kLockTypeRead);
			EATEST_VERIFY_MSG(nLockResult == 0, "RWMutex failure");

			// Verify exactly one write lock is set.
			nLockResult = pWorkData->mRWMutex.GetLockCount(RWMutex::kLockTypeWrite);
			EATEST_VERIFY_MSG(nLockResult == 1, "RWMutex failure");

			// Verify there are now zero write locks set.
			nLockResult = pWorkData->mRWMutex.Unlock();
			EATEST_VERIFY_MSG(nLockResult == 0, "RWMutex failure");

			EA::UnitTest::ThreadSleepRandom(400, 800);
		}
	}

	pWorkData->mnErrorCount += nErrorCount;

	return 0;
}


int TestThreadRWMutex()
{
	int nErrorCount(0);

	// Be careful adding when adding more mutexes to this test as the the CTR runs out of 
	// resources pretty early.
	{
		RWMutexParameters mp1(true, NULL);
		RWMutexParameters mp2(true, "mp2");

		{
			RWMutex mutex1(&mp1, false);
			RWMutex mutex2(&mp2, false);
		}

		{
			RWMutex mutex5(NULL, true);
			RWMutex mutex6(NULL, false);
			mutex6.Init(&mp1);
		}

		{
			RWMutex mutex1(&mp1, false);
			AutoRWMutex am1(mutex1, RWMutex::kLockTypeRead);
		}
	}

	#if EA_THREADS_AVAILABLE

		{
			RWMWorkData workData; 

			const int      kThreadCount(kMaxConcurrentThreadCount - 1);
			Thread         thread[kThreadCount];
			ThreadId       threadId[kThreadCount];
			Thread::Status status;

			for(int i(0); i < kThreadCount; i++)
			{
				if(i < (kThreadCount * 3 / 4))
					threadId[i] = thread[i].Begin(ReaderFunction, &workData);
				else
					threadId[i] = thread[i].Begin(WriterFunction, &workData);
			}

			EA::UnitTest::ThreadSleepRandom(gTestLengthSeconds * 1000, gTestLengthSeconds * 1000);

			workData.mbShouldQuit = true;

			for(int i(0); i < kThreadCount; i++)
			{
				if(threadId[i] != kThreadIdInvalid)
				{
					status = thread[i].WaitForEnd(GetThreadTime() + 30000);
					EATEST_VERIFY_MSG(status != Thread::kStatusRunning, "RWMutex/Thread failure: status == kStatusRunning.");
				}
			}

			nErrorCount += (int)workData.mnErrorCount;
		}

	#endif

	return nErrorCount;
}

