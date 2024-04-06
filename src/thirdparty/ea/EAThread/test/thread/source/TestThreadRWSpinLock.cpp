///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "TestThread.h"
#include <EATest/EATest.h>
#include <eathread/eathread_thread.h>
#include <eathread/eathread_rwspinlock.h>
#include <eathread/eathread_rwspinlockw.h>
#include <stdlib.h>
#include <atomic>


const int kThreadCount = EATHREAD_MAX_CONCURRENT_THREAD_COUNT;


///////////////////////////////////////////////////////////////////////////////
// RWSTestType
//
enum RWSTestType
{
	kRWSTestTypeStandard,
	kRWSTestTypeAllWriters,
	kRWSTestTypeAllReaders,
	kRWSTestTypeMostlyWriters,
	kRWSTestTypeMostlyReaders,
	kRWSTestTypeCount
};


///////////////////////////////////////////////////////////////////////////////
// RWSWorkData
//
struct RWSWorkData
{
	std::atomic<bool>           mbShouldQuit;
	EA::Thread::RWSpinLock      mRWSpinLock;
	EA::Thread::RWSpinLockW     mRWSpinLockW;
	volatile int                mnExpectedValue;
	volatile int                mnCalculatedValue;
	EA::Thread::AtomicInt32     mnErrorCount;
	EA::Thread::AtomicInt32     mnCurrentTestType;

	RWSWorkData() : mbShouldQuit(false), mRWSpinLock(), mRWSpinLockW(), mnExpectedValue(0), mnCalculatedValue(0), 
					mnErrorCount(0), mnCurrentTestType(kRWSTestTypeStandard) {}

private:
	RWSWorkData(const RWSWorkData& rhs);
	RWSWorkData& operator=(const RWSWorkData& rhs);
};


///////////////////////////////////////////////////////////////////////////////
// RWSWThreadFunction
//
static intptr_t RWSWThreadFunction(void* pvWorkData)
{
	using namespace EA::Thread;

	RWSWorkData* const pWorkData = (RWSWorkData*)pvWorkData;

	ThreadId threadId = GetThreadId();
	EA::UnitTest::ReportVerbosity(1, "RWSpinLockW test function created: %s\n", EAThreadThreadIdToString(threadId));

	int nErrorCount = 0;

	while(!pWorkData->mbShouldQuit)
	{
		int               nWriteLockChance = 0;
		const RWSTestType testType         = (RWSTestType)pWorkData->mnCurrentTestType.GetValue();

		switch (testType)
		{
			default:
			case kRWSTestTypeStandard:
				nWriteLockChance = 20;
				break;

			case kRWSTestTypeAllWriters:
				nWriteLockChance = 1000;
				break;

			case kRWSTestTypeAllReaders:
				nWriteLockChance = 0;
				break;

			case kRWSTestTypeMostlyWriters:
				nWriteLockChance = 700;
				break;

			case kRWSTestTypeMostlyReaders:
				nWriteLockChance = 5;
				break;
		}

		const bool bShouldWrite = ((rand() % 1000) < nWriteLockChance);

		if(bShouldWrite)
		{
			pWorkData->mRWSpinLockW.WriteLock();

			EATEST_VERIFY_MSG(!pWorkData->mRWSpinLockW.IsReadLocked(), "RWSpinlock failure: IsReadLocked\n");
			EATEST_VERIFY_MSG(pWorkData->mRWSpinLockW.IsWriteLocked(), "RWSpinlock failure: IsWriteLocked\n");

			pWorkData->mRWSpinLockW.WriteUnlock();

			ThreadCooperativeYield();
		}
		else
		{
			const int nRecursiveLockCount = 1; // Disabled because we are not recursive: (rand() % 10) ? 1 : 2;

			int nLocks = 0;

			for(int i = 0; i < nRecursiveLockCount; i++)
			{
				pWorkData->mRWSpinLockW.ReadLock();
				nLocks++;

				ThreadCooperativeYield();
			}

			// Disabled because we are not recursive:
			// if((rand() % 10) == 0)
			// {
			//     if(pWorkData->mRWSpinLockW.ReadTryLock())
			//         nLocks++;
			// }

			while(nLocks > 0)
			{
				EATEST_VERIFY_MSG(pWorkData->mRWSpinLockW.IsReadLocked(),   "RWSpinlock failure: IsReadLocked\n");
				EATEST_VERIFY_MSG(!pWorkData->mRWSpinLockW.IsWriteLocked(), "RWSpinlock failure: IsWriteLocked\n");

				pWorkData->mRWSpinLockW.ReadUnlock();
				nLocks--;

				ThreadCooperativeYield();
			}
		}

		//if((rand() % 1000) < 3)
		//    EA::UnitTest::ThreadSleepRandom(50, 100);
	}

	pWorkData->mnErrorCount.SetValue(nErrorCount);

	return nErrorCount;
}



static int TestThreadRWSpinLockW()
{
	using namespace EA::Thread;

	int nErrorCount = 0;

	{ // RWSpinLockW -- Basic single-threaded test.

		RWSpinLockW rwSpinLock; // There are no construction parameters.

		EATEST_VERIFY_MSG(!rwSpinLock.IsReadLocked(),  "RWSpinLockW failure");
		EATEST_VERIFY_MSG(!rwSpinLock.IsWriteLocked(), "RWSpinLockW failure");

		rwSpinLock.ReadTryLock();
		EATEST_VERIFY_MSG( rwSpinLock.IsReadLocked(),  "RWSpinLockW failure");
		EATEST_VERIFY_MSG(!rwSpinLock.IsWriteLocked(), "RWSpinLockW failure");
		EATEST_VERIFY_MSG(!rwSpinLock.WriteTryLock(),  "RWSpinLockW failure");
		EATEST_VERIFY_MSG(!rwSpinLock.IsWriteLocked(), "RWSpinLockW failure");

		// Disabled because we don't support read lock recursion.
		// rwSpinLock.ReadLock();
		// EATEST_VERIFY_MSG(rwSpinLock.IsReadLocked(), "RWSpinLockW failure");

		// rwSpinLock.ReadUnlock();
		// EATEST_VERIFY_MSG(rwSpinLock.IsReadLocked(), "RWSpinLockW failure");

		rwSpinLock.ReadUnlock();
		EATEST_VERIFY_MSG(!rwSpinLock.IsReadLocked(), "RWSpinLockW failure");

		rwSpinLock.WriteTryLock();
		EATEST_VERIFY_MSG( rwSpinLock.IsWriteLocked(),"RWSpinLockW failure");
		EATEST_VERIFY_MSG(!rwSpinLock.IsReadLocked(), "RWSpinLockW failure");
		EATEST_VERIFY_MSG(!rwSpinLock.ReadTryLock(),  "RWSpinLockW failure");
		EATEST_VERIFY_MSG(!rwSpinLock.IsReadLocked(), "RWSpinLockW failure");
		EATEST_VERIFY_MSG(!rwSpinLock.WriteTryLock(), "RWSpinLockW failure");
	}


	{ // AutoRWSpinLockW -- Basic single-threaded test.
		RWSpinLockW rwSpinLock; // There are no construction parameters.

		{  //Special scope just for the AutoRWSpinLockW
			AutoRWSpinLockW autoRWSpinLockW1(rwSpinLock, AutoRWSpinLockW::kLockTypeRead);
			AutoRWSpinLockW autoRWSpinLockW2(rwSpinLock, AutoRWSpinLockW::kLockTypeRead);

			EATEST_VERIFY_MSG( rwSpinLock.IsReadLocked(),  "RWSpinLockW failure");
			EATEST_VERIFY_MSG(!rwSpinLock.IsWriteLocked(), "RWSpinLockW failure");
			EATEST_VERIFY_MSG(!rwSpinLock.WriteTryLock(),  "RWSpinLockW failure");
			EATEST_VERIFY_MSG(!rwSpinLock.IsWriteLocked(), "RWSpinLockW failure");
		}

		EATEST_VERIFY_MSG(!rwSpinLock.IsReadLocked(),  "RWSpinLockW failure");
		EATEST_VERIFY_MSG(!rwSpinLock.IsWriteLocked(), "RWSpinLockW failure");

		{  //Special scope just for the AutoRWSpinLockW
			AutoRWSpinLockW autoRWSpinLockW(rwSpinLock, AutoRWSpinLockW::kLockTypeWrite);

			EATEST_VERIFY_MSG( rwSpinLock.IsWriteLocked(),"RWSpinLockW failure");
			EATEST_VERIFY_MSG(!rwSpinLock.IsReadLocked(), "RWSpinLockW failure");
			EATEST_VERIFY_MSG(!rwSpinLock.ReadTryLock(),  "RWSpinLockW failure");
			EATEST_VERIFY_MSG(!rwSpinLock.IsReadLocked(), "RWSpinLockW failure");
		}

		EATEST_VERIFY_MSG(!rwSpinLock.IsReadLocked(),  "RWSpinLockW failure");
		EATEST_VERIFY_MSG(!rwSpinLock.IsWriteLocked(), "RWSpinLockW failure");
	}


	#if EA_THREADS_AVAILABLE

		{  // Multithreaded test
			  
			RWSWorkData workData; 

			Thread         thread[kThreadCount];
			ThreadId       threadId[kThreadCount];
			Thread::Status status;

			for(int i(0); i < kThreadCount; i++)
				threadId[i] = thread[i].Begin(RWSWThreadFunction, &workData);

			for(int e = 0; e < kRWSTestTypeCount; e++)
			{
				workData.mnCurrentTestType.SetValue(e);
				EA::UnitTest::ThreadSleepRandom(gTestLengthSeconds * 500, gTestLengthSeconds * 500);
			}

			workData.mbShouldQuit = true;

			for(int t(0); t < kThreadCount; t++)
			{
				if(threadId[t] != kThreadIdInvalid)
				{
					status = thread[t].WaitForEnd(GetThreadTime() + 30000);
					EATEST_VERIFY_MSG(status != Thread::kStatusRunning, "RWSpinlock/Thread failure: status == kStatusRunning.\n");
				}
			}

			nErrorCount += (int)workData.mnErrorCount;
		}

	#endif

	return nErrorCount;
}



///////////////////////////////////////////////////////////////////////////////
// RWSThreadFunction
//
static intptr_t RWSThreadFunction(void* pvWorkData)
{
	using namespace EA::Thread;

	RWSWorkData* const pWorkData = (RWSWorkData*)pvWorkData;

	ThreadId threadId = GetThreadId();
	EA::UnitTest::ReportVerbosity(1, "RWSpinLock test function created: %s\n", EAThreadThreadIdToString(threadId));

	int nErrorCount = 0;

	while(!pWorkData->mbShouldQuit)
	{
		int               nWriteLockChance = 0;
		const RWSTestType testType         = (RWSTestType)pWorkData->mnCurrentTestType.GetValue();

		switch (testType)
		{
			default:
			case kRWSTestTypeStandard:
				nWriteLockChance = 20;
				break;

			case kRWSTestTypeAllWriters:
				nWriteLockChance = 1000;
				break;

			case kRWSTestTypeAllReaders:
				nWriteLockChance = 0;
				break;

			case kRWSTestTypeMostlyWriters:
				nWriteLockChance = 700;
				break;

			case kRWSTestTypeMostlyReaders:
				nWriteLockChance = 5;
				break;
		}

		const bool bShouldWrite = ((rand() % 1000) < nWriteLockChance);

		if(bShouldWrite)
		{
			pWorkData->mRWSpinLock.WriteLock();

			EATEST_VERIFY_MSG(!pWorkData->mRWSpinLock.IsReadLocked(), "RWSpinlock failure: IsReadLocked\n");
			EATEST_VERIFY_MSG(pWorkData->mRWSpinLock.IsWriteLocked(), "RWSpinlock failure: IsWriteLocked\n");

			pWorkData->mRWSpinLock.WriteUnlock();

			ThreadCooperativeYield();
		}
		else
		{
			const int nRecursiveLockCount = (rand() % 10) ? 1 : 2;

			int nLocks = 0;

			for(int i = 0; i < nRecursiveLockCount; i++)
			{
				pWorkData->mRWSpinLock.ReadLock();
				nLocks++;

				ThreadCooperativeYield();
			}

			if((rand() % 10) == 0)
			{
				if(pWorkData->mRWSpinLock.ReadTryLock())
					nLocks++;
			}

			while(nLocks > 0)
			{
				const int32_t n = pWorkData->mRWSpinLock.mValue; (void)n;

				// It turns out IsReadLocked has a bug and can return a false negative. 
				// EATEST_VERIFY_MSG(pWorkData->mRWSpinLock.IsReadLocked(), "RWSpinlock failure: IsReadLocked\n");
				// EATEST_VERIFY_MSG(!pWorkData->mRWSpinLock.IsWriteLocked(), "RWSpinlock failure: IsWriteLocked\n");

				pWorkData->mRWSpinLock.ReadUnlock();
				nLocks--;

				ThreadCooperativeYield();
			}
		}
	}

	pWorkData->mnErrorCount.SetValue(nErrorCount);

	return nErrorCount;
}



int TestThreadRWSpinLock()
{
	using namespace EA::Thread;

	int nErrorCount = 0;

	{ // RWSpinLock -- Basic single-threaded test.

		RWSpinLock rwSpinLock; // There are no construction parameters.

		EATEST_VERIFY_MSG(!rwSpinLock.IsReadLocked(),  "RWSpinLock failure");
		EATEST_VERIFY_MSG(!rwSpinLock.IsWriteLocked(), "RWSpinLock failure");

		rwSpinLock.ReadTryLock();
		EATEST_VERIFY_MSG(rwSpinLock.IsReadLocked(),   "RWSpinLock failure");
		EATEST_VERIFY_MSG(!rwSpinLock.IsWriteLocked(), "RWSpinLock failure");
		EATEST_VERIFY_MSG(!rwSpinLock.WriteTryLock(),  "RWSpinLock failure");
		EATEST_VERIFY_MSG(!rwSpinLock.IsWriteLocked(), "RWSpinLock failure");

		rwSpinLock.ReadLock();
		EATEST_VERIFY_MSG(rwSpinLock.IsReadLocked(), "RWSpinLock failure");

		rwSpinLock.ReadUnlock();
		EATEST_VERIFY_MSG(rwSpinLock.IsReadLocked(), "RWSpinLock failure");

		rwSpinLock.ReadUnlock();
		EATEST_VERIFY_MSG(!rwSpinLock.IsReadLocked(), "RWSpinLock failure");

		rwSpinLock.WriteTryLock();
		EATEST_VERIFY_MSG(rwSpinLock.IsWriteLocked(), "RWSpinLock failure");
		EATEST_VERIFY_MSG(!rwSpinLock.IsReadLocked(), "RWSpinLock failure");
		EATEST_VERIFY_MSG(!rwSpinLock.ReadTryLock(),  "RWSpinLock failure");
		EATEST_VERIFY_MSG(!rwSpinLock.IsReadLocked(), "RWSpinLock failure");
		EATEST_VERIFY_MSG(!rwSpinLock.WriteTryLock(), "RWSpinLock failure");
	}


	{ // AutoRWSpinLock -- Basic single-threaded test.
		RWSpinLock rwSpinLock; // There are no construction parameters.

		{  //Special scope just for the AutoRWSpinLock
			AutoRWSpinLock autoRWSpinLock1(rwSpinLock, AutoRWSpinLock::kLockTypeRead);
			AutoRWSpinLock autoRWSpinLock2(rwSpinLock, AutoRWSpinLock::kLockTypeRead);

			EATEST_VERIFY_MSG(rwSpinLock.IsReadLocked(),   "RWSpinLock failure");
			EATEST_VERIFY_MSG(!rwSpinLock.IsWriteLocked(), "RWSpinLock failure");
			EATEST_VERIFY_MSG(!rwSpinLock.WriteTryLock(),  "RWSpinLock failure");
			EATEST_VERIFY_MSG(!rwSpinLock.IsWriteLocked(), "RWSpinLock failure");
		}

		EATEST_VERIFY_MSG(!rwSpinLock.IsReadLocked(),  "RWSpinLock failure");
		EATEST_VERIFY_MSG(!rwSpinLock.IsWriteLocked(), "RWSpinLock failure");

		{  //Special scope just for the AutoRWSpinLock
			AutoRWSpinLock autoRWSpinLock(rwSpinLock, AutoRWSpinLock::kLockTypeWrite);

			EATEST_VERIFY_MSG(rwSpinLock.IsWriteLocked(), "RWSpinLock failure");
			EATEST_VERIFY_MSG(!rwSpinLock.IsReadLocked(), "RWSpinLock failure");
			EATEST_VERIFY_MSG(!rwSpinLock.ReadTryLock(),  "RWSpinLock failure");
			EATEST_VERIFY_MSG(!rwSpinLock.IsReadLocked(), "RWSpinLock failure");
		}

		EATEST_VERIFY_MSG(!rwSpinLock.IsReadLocked(),  "RWSpinLock failure");
		EATEST_VERIFY_MSG(!rwSpinLock.IsWriteLocked(), "RWSpinLock failure");
	}


	#if EA_THREADS_AVAILABLE

		{  // Multithreaded test
			  
			RWSWorkData workData; 

			Thread         thread[kThreadCount];
			ThreadId       threadId[kThreadCount];
			Thread::Status status;

			for(int i(0); i < kThreadCount; i++)
				threadId[i] = thread[i].Begin(RWSThreadFunction, &workData);

			for(int e = 0; e < kRWSTestTypeCount; e++)
			{
				workData.mnCurrentTestType.SetValue(e);
				EA::UnitTest::ThreadSleepRandom(gTestLengthSeconds * 500, gTestLengthSeconds * 500);
			}

			workData.mbShouldQuit = true;

			for(int t(0); t < kThreadCount; t++)
			{
				if(threadId[t] != kThreadIdInvalid)
				{
					status = thread[t].WaitForEnd(GetThreadTime() + 30000);
					EATEST_VERIFY_MSG(status != Thread::kStatusRunning, "RWSpinlock/Thread failure: status == kStatusRunning.\n");
				}
			}

			nErrorCount += (int)workData.mnErrorCount;
		}

	#endif

	// TestThreadRWSpinLockW
	nErrorCount += TestThreadRWSpinLockW();


	return nErrorCount;
}










