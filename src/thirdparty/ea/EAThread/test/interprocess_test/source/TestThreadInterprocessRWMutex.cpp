///////////////////////////////////////////////////////////////////////////////
// TestThreadInterprocessRWMutex.cpp
//
// Copyright (c) 2009, Electronic Arts Inc. All rights reserved.
// Created by Paul Pedriana
///////////////////////////////////////////////////////////////////////////////


#include <EATest/EATest.h>
#include <eathread/eathread.h>
#include <eathread/eathread_thread.h>
#include <eathread/eathread_atomic.h>
#include <eathread/eathread_rwmutex_ip.h>
#include "TestThreadInterprocess.h"
#include <stdlib.h>


struct RWMWorkDataInterProcess
{
	volatile int mnExpectedValue;     // Intentionally not an atomic variable.
	volatile int mnCalculatedValue;   // Intentionally not an atomic variable.
	volatile int mnWriteLockCount;    // How many times the write lock was owned, across all processes.

	RWMWorkDataInterProcess()
	  : mnExpectedValue(0),
		mnCalculatedValue(0),
		mnWriteLockCount(0)
	{
		printf("RWMWorkDataInterProcess\n");
	}

   ~RWMWorkDataInterProcess()
	{
		printf("~RWMWorkDataInterProcess\n");
	}
};


struct RWMWorkDataInterThread
{
	volatile bool           mbShouldQuit;           // 
	EA::Thread::RWMutexIP   mRWMutexIP;             // 
	EA::Thread::AtomicInt32 mnThreadIndex;          // 
	EA::Thread::AtomicInt32 mnErrorCount;           // 
	EA::Thread::AtomicInt32 mnReadLockCount;        // How many times the read lock was owned, within this process.
	EA::Thread::AtomicInt32 mnWriteLockCount;       // How many times the write lock was owned, within this process.

	RWMWorkDataInterThread() 
	  : mbShouldQuit(false),
		mRWMutexIP(NULL, false),
		mnThreadIndex(0),
		mnErrorCount(0),
		mnReadLockCount(0),
		mnWriteLockCount(0)
	{
	}

protected:
	RWMWorkDataInterThread(const RWMWorkDataInterThread& rhs);
	RWMWorkDataInterThread& operator=(const RWMWorkDataInterThread& rhs);
};


static intptr_t RWThreadFunction(void* pvWorkData)
{
	using namespace EA::Thread;

	int                     nErrorCount = 0;
	RWMWorkDataInterThread* pWorkData   = (RWMWorkDataInterThread*)pvWorkData;
	ThreadId                threadId    = GetThreadId();

	EA::UnitTest::ReportVerbosity(1, "RWMutexIP test function created: %08x\n", (int)(intptr_t)threadId);

	// We use the interprocess mutex to control access to an interprocess data struct.
	Shared<RWMWorkDataInterProcess> gSharedData("RWMWorkDataIP");

	// We track the amount of time we spend waiting for Locks.
	//ThreadTime nInitialTime, nFinalTime;
	const ThreadTime kMaxExpectedTime = 1000;

	while(!pWorkData->mbShouldQuit)
	{
		const bool bWriteLock((rand() % 10) == 0);   // 10% of the time, do a write lock.

		if(bWriteLock)
		{
			//nInitialTime = EA::Thread::GetThreadTime();

			int nLockResult = pWorkData->mRWMutexIP.Lock(RWMutexIP::kLockTypeWrite, GetThreadTime() + kMaxExpectedTime);
			EATEST_VERIFY_MSG(nLockResult != RWMutexIP::kResultError, "RWMutexIP failure: write lock.");

			//nFinalTime = EA::Thread::GetThreadTime();
			//EATEST_VERIFY_MSG((nFinalTime - nInitialTime) < kMaxExpectedTime, "RWMutexIP failure: write lock slow.");

			if(nLockResult > 0)
			{
				gSharedData->mnWriteLockCount++;
				pWorkData->mnWriteLockCount++;

				// Verify exactly one write lock is set.
				nLockResult = pWorkData->mRWMutexIP.GetLockCount(RWMutexIP::kLockTypeWrite);
				EATEST_VERIFY_MSG(nLockResult == 1, "RWMutexIP failure: write lock verify 1.");

				// What we do here is spend some time manipulating mnExpectedValue and mnCalculatedValue
				// while we have the write lock. We change their values in a predicable way but before 
				// we are done mnCalculatedValue has been incremented by one and both values are equal.
				const uintptr_t x = (uintptr_t)pWorkData;

				gSharedData->mnExpectedValue    = -1;
				EA::UnitTest::ThreadSleepRandom(10, 20);
				gSharedData->mnCalculatedValue *= 50;
				EA::UnitTest::ThreadSleepRandom(10, 20);
				gSharedData->mnCalculatedValue /= (int)(((x + 1) / x) * 50); // This will always be the same as simply '/= 50'.
				EA::UnitTest::ThreadSleepRandom(10, 20);
				gSharedData->mnCalculatedValue += 1;
				EA::UnitTest::ThreadSleepRandom(10, 20);
				gSharedData->mnExpectedValue    = gSharedData->mnCalculatedValue;

				// Verify no read locks are set.
				nLockResult = pWorkData->mRWMutexIP.GetLockCount(RWMutexIP::kLockTypeRead);
				EATEST_VERIFY_MSG(nLockResult == 0, "RWMutexIP failure: write lock verify 2.");

				// Verify exactly one write lock is set.
				nLockResult = pWorkData->mRWMutexIP.GetLockCount(RWMutexIP::kLockTypeWrite);
				EATEST_VERIFY_MSG(nLockResult == 1, "RWMutexIP failure: write lock verify 3.");

				// Verify there are now zero write locks set.
				nLockResult = pWorkData->mRWMutexIP.Unlock();
				EATEST_VERIFY_MSG(nLockResult == 0, "RWMutexIP failure: write unlock.");

				EA::UnitTest::ThreadSleepRandom(40, 80);
			}
		}
		else
		{
			const int nRecursiveLockCount(rand() % 2);
			int i, nLockResult, nLocks = 0;

			for(i = 0; i < nRecursiveLockCount; i++)
			{
				//nInitialTime = EA::Thread::GetThreadTime();

				nLockResult = pWorkData->mRWMutexIP.Lock(RWMutexIP::kLockTypeRead, GetThreadTime() + kMaxExpectedTime);

				//nFinalTime = EA::Thread::GetThreadTime();
				//EATEST_VERIFY_MSG(nLockResult != RWMutexIP::kResultError, "RWMutexIP failure: read lock.");

				if(nLockResult > 0)
				{
					nLocks++;
					pWorkData->mnReadLockCount++;

					EA::UnitTest::ReportVerbosity(2, "CValue = %d; EValue = %d\n", gSharedData->mnCalculatedValue, gSharedData->mnExpectedValue);
					EATEST_VERIFY_MSG(gSharedData->mnCalculatedValue == gSharedData->mnExpectedValue, "RWMutexIP failure: read lock 2");
				}
			}

			while(nLocks > 0)
			{
				// Verify no write locks are set.
				nLockResult = pWorkData->mRWMutexIP.GetLockCount(RWMutexIP::kLockTypeWrite);
				EATEST_VERIFY_MSG(nLockResult == 0, "RWMutexIP failure: read lock verify 1.");

				// Verify at least N read locks are set.
				nLockResult = pWorkData->mRWMutexIP.GetLockCount(RWMutexIP::kLockTypeRead);
				EATEST_VERIFY_MSG(nLockResult >= nLocks, "RWMutexIP failure: read lock verify 2.");

				// Verify there is one less read lock set.
				nLockResult = pWorkData->mRWMutexIP.Unlock();
				EATEST_VERIFY_MSG(nLockResult >= nLocks-1, "RWMutexIP failure: read unlock.");

				nLocks--;
			}

			EA::UnitTest::ThreadSleepRandom(10, 20);
		}
	}

	pWorkData->mnErrorCount += nErrorCount;

	return 0;
}



int TestThreadRWMutex()
{
	using namespace EA::Thread;

	int nErrorCount(0);

	EA::UnitTest::Report("Thread Pool Test\n");

	/*
	{ // ctor tests
		// We test various combinations of RWMutexIP ctor and RWMutexIPParameters.
		// RWMutexIPParameters(bool bIntraProcess = true, const char* pName = NULL);
		// RWMutexIP(const RWMutexIPParameters* pRWMutexIPParameters = NULL, bool bDefaultParameters = true);

	  //RWMutexIPParameters mp1(true,   NULL);
	  //RWMutexIPParameters mp2(true,  "mp2");
	  //RWMutexIPParameters mp3(false, "mp3");
		RWMutexIPParameters mp4(false, "mp4");
		RWMutexIPParameters mp6(false, "mp6");

	  //RWMutexIP mutex1(&mp1, false);
	  //RWMutexIP mutex2(&mp2, false);
	  //RWMutexIP mutex3(&mp3, false);
		RWMutexIP mutex4(&mp4, false);
	  //RWMutexIP mutex5(NULL, true);
		RWMutexIP mutex6(NULL, false);
		mutex6.Init(&mp6);

	  //AutoRWMutexIP am1(mutex1, RWMutexIP::kLockTypeRead);
	  //AutoRWMutexIP am2(mutex2, RWMutexIP::kLockTypeRead);
	  //AutoRWMutexIP am3(mutex3, RWMutexIP::kLockTypeRead);
		AutoRWMutexIP am4(mutex4, RWMutexIP::kLockTypeRead);
		AutoRWMutexIP am6(mutex6, RWMutexIP::kLockTypeRead);
	}
	*/

	{
		RWMWorkDataInterThread workData;
		RWMutexIPParameters    rwMutexIPParameters(false, "RWMTest"); 

		// Set up the RWMWorkData
		workData.mRWMutexIP.Init(&rwMutexIPParameters);

		// Create the threads
		Thread*        pThreadArray   = new Thread[gTestThreadCount];
		ThreadId*      pThreadIdArray = new ThreadId[gTestThreadCount];
		Thread::Status status;

		for(unsigned i(0); i < gTestThreadCount; i++)
			pThreadIdArray[i] = pThreadArray[i].Begin(RWThreadFunction, &workData);

		EA::UnitTest::ThreadSleepRandom(gTestLengthSeconds * 1000, gTestLengthSeconds * 1000);

		workData.mbShouldQuit = true;

		for(unsigned i(0); i < gTestThreadCount; i++)
		{
			if(pThreadIdArray[i] != kThreadIdInvalid)
			{
				status = pThreadArray[i].WaitForEnd(GetThreadTime() + 30000);

				EATEST_VERIFY_MSG(status != Thread::kStatusRunning, "RWMutexIP/Thread failure: status == kStatusRunning.");
			}
		}

		delete[] pThreadIdArray;
		delete[] pThreadArray;

		nErrorCount += (int)workData.mnErrorCount;
	}

	return nErrorCount;
}

