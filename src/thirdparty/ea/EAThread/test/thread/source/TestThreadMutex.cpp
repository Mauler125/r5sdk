///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "TestThread.h"
#include <EATest/EATest.h>
#include <eathread/eathread_mutex.h>
#include <eathread/eathread_thread.h>
#include <eathread/eathread_atomic.h>
#include <stdlib.h>


using namespace EA::Thread;


const int kMaxConcurrentThreadCount = EATHREAD_MAX_CONCURRENT_THREAD_COUNT;


struct MWorkData
{
	volatile bool mbShouldQuit;
	Mutex         mMutex;
	volatile int  mnExpectedValue;
	volatile int  mnCalculatedValue;
	AtomicInt32   mnErrorCount;

	MWorkData() : mbShouldQuit(false), mMutex(NULL, true), 
				  mnExpectedValue(0), mnCalculatedValue(0), 
				  mnErrorCount(0) {}

	// Prevent default generation of these functions by not defining them
private:
	MWorkData(const MWorkData& rhs);               // copy constructor
	MWorkData& operator=(const MWorkData& rhs);    // assignment operator
};


static intptr_t MutexTestThreadFunction(void* pvWorkData)
{
	int            nErrorCount      = 0;
	MWorkData*     pWorkData        = (MWorkData*)pvWorkData;
	const ThreadId threadId         = GetThreadId();
	const int      currentProcessor = GetThreadProcessor();

	EA::UnitTest::ReportVerbosity(1, "Mutex test function created: %s (currently on processor %d).\n", EAThreadThreadIdToString(threadId), currentProcessor);

	while(!pWorkData->mbShouldQuit)
	{
		const int nRecursiveLockCount(rand() % 3);
		int i, nLockResult, nLocks = 0;

		for(i = 0; i < nRecursiveLockCount; i++)
		{
			// Do a lock but allow for the possibility of occasional timeout.
			ThreadTime expectedTime(GetThreadTime() + 1000);
			nLockResult = pWorkData->mMutex.Lock(expectedTime);

			// Verify the lock succeeded or timed out.
			EATEST_VERIFY_MSG(nLockResult != Mutex::kResultError, "Mutex failure.");

			// Verify the timeout of the lock
			if (nLockResult == Mutex::kResultTimeout)
			{
				ThreadTime currentTime(GetThreadTime());
				EATEST_VERIFY_MSG(currentTime >= expectedTime, "Mutex timeout failure.");
			}
	 
			if(nLockResult > 0) // If there was no timeout...
			{
				nLocks++;

				// What we do here is spend some time manipulating mnExpectedValue and mnCalculatedValue
				// while we have the lock. We change their values in a predicable way but before we 
				// are done mnCalculatedValue has been incremented by one and both values are equal.
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

				EATEST_VERIFY_MSG(pWorkData->mnCalculatedValue == pWorkData->mnExpectedValue, "Mutex failure.");
			}
			ThreadCooperativeYield(); // Used by cooperative threading platforms.
		}

		while(nLocks > 0)
		{
			// Verify that HasLock returns the expected value.
			EATEST_VERIFY_MSG(pWorkData->mMutex.HasLock(), "Mutex failure.");

			// Verify that N locks are set.
			nLockResult = pWorkData->mMutex.GetLockCount();
			EATEST_VERIFY_MSG(nLockResult == nLocks, "Mutex failure.");

			// Verify the unlock result.
			nLockResult = pWorkData->mMutex.Unlock();
			EATEST_VERIFY_MSG(nLockResult >= nLocks-1, "Mutex failure.");

			nLocks--;
			ThreadCooperativeYield(); // Used by cooperative threading platforms.
		}

		// If EAT_ASSERT_ENABLED is 1, Mutex::HasLock() tests to see that that not only is the mutex 
		// locked, but that the lock is owned by the calling thread.
		#if EAT_ASSERT_ENABLED
			// Verify that HasLock returns the expected value.
			EATEST_VERIFY_MSG(!pWorkData->mMutex.HasLock(), "Mutex failure.");
		#endif

		EA::UnitTest::ThreadSleepRandom(100, 200);
	}

	pWorkData->mnErrorCount += nErrorCount;

	return 0;
}


int TestThreadMutex()
{
	int nErrorCount(0);

	{ // ctor tests
		// We test various combinations of Mutex ctor and MutexParameters.
		// MutexParameters(bool bIntraProcess = true, const char* pName = NULL);
		// Mutex(const MutexParameters* pMutexParameters = NULL, bool bDefaultParameters = true);

		MutexParameters mp1(true, NULL);
		MutexParameters mp2(true, "mp2");
		MutexParameters mp3(false, NULL);
		MutexParameters mp4(false, "mp4WithNameThatIsMoreThan32Characters"); // testing kettle mutex name limitations

		Mutex mutex1(&mp1, false);
		Mutex mutex2(&mp2, false);
		Mutex mutex3(&mp3, false);
		Mutex mutex4(&mp4, false);
		Mutex mutex5(NULL, true);
		Mutex mutex6(NULL, false);
		mutex6.Init(&mp1);

		AutoMutex am1(mutex1);
		AutoMutex am2(mutex2);
		AutoMutex am3(mutex3);
		AutoMutex am4(mutex4);
		AutoMutex am5(mutex5);
		AutoMutex am6(mutex6);
	}

	{ // Single-threaded tests
		Mutex mutex(NULL, true);

		int nLockCount;

		nLockCount = mutex.GetLockCount();
		EATEST_VERIFY_MSG(nLockCount == 0, "Mutex failure.");

		nLockCount = mutex.Lock();
		EATEST_VERIFY_MSG(nLockCount == 1, "Mutex failure.");

		nLockCount = mutex.Lock();
		EATEST_VERIFY_MSG(nLockCount == 2, "Mutex failure.");

		nLockCount = mutex.Unlock();
		EATEST_VERIFY_MSG(nLockCount == 1, "Mutex failure.");

		nLockCount = mutex.GetLockCount();
		EATEST_VERIFY_MSG(nLockCount == 1, "Mutex failure.");

		nLockCount = mutex.Unlock();
		EATEST_VERIFY_MSG(nLockCount == 0, "Mutex failure.");

		nLockCount = mutex.Lock();
		EATEST_VERIFY_MSG(nLockCount == 1, "Mutex failure.");

		nLockCount = mutex.Unlock();
		EATEST_VERIFY_MSG(nLockCount ==  0, "Mutex failure.");
	}

	#ifdef EA_PLATFORM_PS4
	{
		// Validate the amount of system resources being consumed by a Sony Mutex without a mutex name.  It appears the
		// Sony OS allocates 32 bytes per mutex regardless if the mutex name is empty or not.  EAThread currently checks
		// if the mutex name can be omited in an effort to save memory.
		MutexParameters mp(false, nullptr); 

		Mutex mutexes[4000];
		for(auto& m : mutexes)
			m.Init(&mp);
	}
	#endif

	#if EA_THREADS_AVAILABLE

		{  // Multithreaded test
			MWorkData workData; 

			const int kThreadCount(kMaxConcurrentThreadCount);
			Thread thread[kThreadCount];
			Thread::Status status;
			int i;

			for(i = 0; i < kThreadCount; i++)
				thread[i].Begin(MutexTestThreadFunction, &workData);

			EA::UnitTest::ThreadSleepRandom(gTestLengthSeconds*1000, gTestLengthSeconds*1000);

			workData.mbShouldQuit = true;

			for(i = 0; i < kThreadCount; i++)
			{
				status = thread[i].WaitForEnd(GetThreadTime() + 30000);

				EATEST_VERIFY_MSG(status == EA::Thread::Thread::kStatusEnded, "Mutex/Thread failure: Thread(s) didn't end.");
			}

			nErrorCount += (int)workData.mnErrorCount;
		}

	#endif

	return nErrorCount;
}





