///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#include "TestThread.h"
#include <EATest/EATest.h>
#include <eathread/eathread_thread.h>
#include <eathread/eathread_rwsemalock.h>
#include <stdlib.h>



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
// RWSemaWorkData
//
struct RWSemaWorkData
{
	volatile bool               mbShouldQuit;
	EA::Thread::RWSemaLock      mRWSemaLock;
	volatile int                mnWriterCount;
	EA::Thread::AtomicInt32     mnErrorCount;
	EA::Thread::AtomicInt32     mnCurrentTestType;

	RWSemaWorkData()
		: mbShouldQuit(false)
		, mRWSemaLock()
		, mnWriterCount(0)
		, mnErrorCount(0)
		, mnCurrentTestType(kRWSTestTypeStandard) {}

private:
	RWSemaWorkData(const RWSemaWorkData& rhs);
	RWSemaWorkData& operator=(const RWSemaWorkData& rhs);
};

///////////////////////////////////////////////////////////////////////////////
// RWSThreadFunction
//
static intptr_t RWSThreadFunction(void* pvWorkData)
{
	using namespace EA::Thread;

	RWSemaWorkData* const pWorkData = (RWSemaWorkData*)pvWorkData;

	ThreadId threadId = GetThreadId();
	EA::UnitTest::ReportVerbosity(1, "RWSemaLock test function created: %s\n", EAThreadThreadIdToString(threadId));

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
			AutoSemaWriteLock _(pWorkData->mRWSemaLock);
			pWorkData->mnWriterCount++;
			EA::UnitTest::ThreadSleepRandom(2, 10);
			pWorkData->mnWriterCount--;
		}
		else
		{
			AutoSemaReadLock _(pWorkData->mRWSemaLock);
			EATEST_VERIFY_MSG(pWorkData->mnWriterCount == 0, "ReadLock is held, there should be no active WriteLocks.");
		}
	}

	pWorkData->mnErrorCount.SetValue(nErrorCount);

	return nErrorCount;
}


// NOTE(rparolin):
// This exists to introduce test-only functionality for the RWSemaLock.  We can add these functions here because we
// guarantee they will not be called in a concurrent context and they simplify validation of assumption of the lock.
struct TestRWSemaLock : public EA::Thread::RWSemaLock
{
	TestRWSemaLock() = default;
	TestRWSemaLock(const TestRWSemaLock&) = delete;
	TestRWSemaLock(TestRWSemaLock&&) = delete;
	TestRWSemaLock& operator=(const TestRWSemaLock&) = delete;
	TestRWSemaLock& operator=(TestRWSemaLock&&) = delete;

	bool IsReadLocked()
	{
		Status status;
		status.data = mStatus.GetValue();
		return status.readers > 0;
	}

	bool IsWriteLocked()
	{
		Status status;
		status.data = mStatus.GetValue();
		return status.writers > 0;
	}
};


int TestThreadRWSemaLock()
{
	using namespace EA::Thread;

	int nErrorCount = 0;

	{ // RWSemaLock -- Basic single-threaded test.

		TestRWSemaLock rwSemaLock; // There are no construction parameters.

		EATEST_VERIFY_MSG(!rwSemaLock.IsReadLocked(),  "RWSemaLock failure");
		EATEST_VERIFY_MSG(!rwSemaLock.IsWriteLocked(), "RWSemaLock failure");

		rwSemaLock.ReadTryLock();
		EATEST_VERIFY_MSG(rwSemaLock.IsReadLocked(),   "RWSemaLock failure");
		EATEST_VERIFY_MSG(!rwSemaLock.IsWriteLocked(), "RWSemaLock failure");
		EATEST_VERIFY_MSG(!rwSemaLock.WriteTryLock(),  "RWSemaLock failure");
		EATEST_VERIFY_MSG(!rwSemaLock.IsWriteLocked(), "RWSemaLock failure");

		rwSemaLock.ReadLock();
		EATEST_VERIFY_MSG(rwSemaLock.IsReadLocked(), "RWSemaLock failure");

		rwSemaLock.ReadUnlock();
		EATEST_VERIFY_MSG(rwSemaLock.IsReadLocked(), "RWSemaLock failure");

		rwSemaLock.ReadUnlock();
		EATEST_VERIFY_MSG(!rwSemaLock.IsReadLocked(), "RWSemaLock failure");

		rwSemaLock.WriteTryLock();
		EATEST_VERIFY_MSG(rwSemaLock.IsWriteLocked(), "RWSemaLock failure");
		EATEST_VERIFY_MSG(!rwSemaLock.IsReadLocked(), "RWSemaLock failure");
		EATEST_VERIFY_MSG(!rwSemaLock.ReadTryLock(),  "RWSemaLock failure");
		EATEST_VERIFY_MSG(!rwSemaLock.IsReadLocked(), "RWSemaLock failure");
		EATEST_VERIFY_MSG(!rwSemaLock.WriteTryLock(), "RWSemaLock failure");
	}


	{ // AutoRWSemaLock -- Basic single-threaded test.
		TestRWSemaLock rwSemaLock; // There are no construction parameters.

		{  //Special scope just for the AutoRWSemaLock
			AutoSemaReadLock autoRWSemaLock1(rwSemaLock);
			AutoSemaReadLock autoRWSemaLock2(rwSemaLock);

			EATEST_VERIFY_MSG(rwSemaLock.IsReadLocked(),   "RWSemaLock failure");
			EATEST_VERIFY_MSG(!rwSemaLock.IsWriteLocked(), "RWSemaLock failure");
			EATEST_VERIFY_MSG(!rwSemaLock.WriteTryLock(),  "RWSemaLock failure");
			EATEST_VERIFY_MSG(!rwSemaLock.IsWriteLocked(), "RWSemaLock failure");
		}

		EATEST_VERIFY_MSG(!rwSemaLock.IsReadLocked(),  "RWSemaLock failure");
		EATEST_VERIFY_MSG(!rwSemaLock.IsWriteLocked(), "RWSemaLock failure");

		{  //Special scope just for the AutoRWSemaLock
			AutoSemaWriteLock autoRWSemaLock(rwSemaLock);

			EATEST_VERIFY_MSG(rwSemaLock.IsWriteLocked(), "RWSemaLock failure");
			EATEST_VERIFY_MSG(!rwSemaLock.IsReadLocked(), "RWSemaLock failure");
			EATEST_VERIFY_MSG(!rwSemaLock.ReadTryLock(),  "RWSemaLock failure");
			EATEST_VERIFY_MSG(!rwSemaLock.IsReadLocked(), "RWSemaLock failure");
		}

		EATEST_VERIFY_MSG(!rwSemaLock.IsReadLocked(),  "RWSemaLock failure");
		EATEST_VERIFY_MSG(!rwSemaLock.IsWriteLocked(), "RWSemaLock failure");
	}


	#if EA_THREADS_AVAILABLE

		{  // Multithreaded test
			  
			RWSemaWorkData workData; 

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
					EATEST_VERIFY_MSG(status != Thread::kStatusRunning, "RWSemalock/Thread failure: status == kStatusRunning.\n");
				}
			}

			nErrorCount += (int)workData.mnErrorCount;
		}

	#endif

	return nErrorCount;
}










