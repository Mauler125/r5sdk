///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "TestThread.h"
#include <EATest/EATest.h>
#include <eathread/eathread_thread.h>
#include <eathread/eathread_mutex.h>
#include <eathread/eathread_barrier.h>
#include <stdlib.h>


using namespace EA::Thread;


const int kMaxConcurrentThreadCount = EATHREAD_MAX_CONCURRENT_THREAD_COUNT;


struct BT_WorkData
{
	Barrier*    mpBarrier; 
	int         mnSleepTime; 
	AtomicInt32 mnErrorCount;

	BT_WorkData(Barrier* pBarrier = NULL, int nSleepTime = 0)
	  : mpBarrier(pBarrier), mnSleepTime(nSleepTime), mnErrorCount(0) {}
};



static intptr_t BT_ConsumerFunction(void* pvWorkData)
{
	int            nErrorCount = 0;
	BT_WorkData*   pWorkData   = (BT_WorkData*)pvWorkData;
	const ThreadId threadId    = GetThreadId();

	EA::UnitTest::ReportVerbosity(1, "Barrier consumer test function created: %s\n", EAThreadThreadIdToString(threadId));

	// Here we would actually do the job, but printing 'job done' is enough in itself.
	ThreadSleep((ThreadTime)pWorkData->mnSleepTime);
	EA::UnitTest::ReportVerbosity(1, "Job done by thread %s.\n", EAThreadThreadIdToString(threadId));
	EA::UnitTest::ReportVerbosity(1, "Start synchronizing by thread %s.\n", EAThreadThreadIdToString(threadId));
	const Barrier::Result result = pWorkData->mpBarrier->Wait(GetThreadTime() + 10000); 

	if(result == Barrier::kResultPrimary)
	{
		// This is the first thread to be released: call producer function
		EA::UnitTest::ReportVerbosity(1, "Serial execution at Barrier by thread %s\n", EAThreadThreadIdToString(threadId));
	}
	else if(result == Barrier::kResultTimeout)
	{
		EA::UnitTest::Report("Barrier time-out by thread %s\n", EAThreadThreadIdToString(threadId));
		nErrorCount++; 
	}
	else if(result == Barrier::kResultError)
	{
		EA::UnitTest::Report("Barrier error in thread %s\n", EAThreadThreadIdToString(threadId));
		nErrorCount++; 
	}

	pWorkData->mnErrorCount += nErrorCount;

	ThreadSleep((ThreadTime)pWorkData->mnSleepTime);

	EA::UnitTest::ReportVerbosity(1, "Job synchronized by thread %s.\n", EAThreadThreadIdToString(threadId));
	return 0;
}


static intptr_t BT_ConsumerFunction2(void * pvWorkData)
{
	((Barrier*)pvWorkData)->Wait();
	((Barrier*)pvWorkData)->Wait();
	return 0;
}


int TestThreadBarrier()
{
	int nErrorCount(0);

	#if EA_THREADS_AVAILABLE

		{
			Thread::Status status;
			const int      kThreadCount(kMaxConcurrentThreadCount - 1);
			BT_WorkData    workData[kThreadCount];
			Thread         threads[kThreadCount];
			ThreadId       threadId[kThreadCount];
			Barrier        barrier(kThreadCount); 
			int            i;

			for(i = 0; i < kThreadCount; i++)
			{
				workData[i].mpBarrier = &barrier;
				workData[i].mnSleepTime = (i + 1) * 500;
			}

			for(i = 0; i < kThreadCount; i++)
			{
				threadId[i] = threads[i].Begin(BT_ConsumerFunction, &workData[i]);
				EATEST_VERIFY_MSG(threadId[i] != kThreadIdInvalid, "Barrier/Thread failure: Couldn't create thread.");
			}

			EA::UnitTest::ThreadSleepRandom(2000,  2000);

			for(i = 0; i < kThreadCount; i++)
			{
				if(threadId[i] != kThreadIdInvalid)
				{
					status = threads[i].WaitForEnd(GetThreadTime() + 30000);
					EATEST_VERIFY_MSG(status != Thread::kStatusRunning, "Barrier/Thread failure.");
				}

				nErrorCount += workData[i].mnErrorCount;
			}
		}

		{
			Thread         threads[2];
			ThreadId       threadId[2];
			Barrier        barrier(2);
			Thread::Status status;

			threadId[0] = threads[0].Begin(BT_ConsumerFunction2, &barrier);
			EATEST_VERIFY_MSG(threadId[0] != kThreadIdInvalid, "Barrier/Thread failure: Couldn't create thread.");

			threadId[1] = threads[1].Begin(BT_ConsumerFunction2, &barrier);
			EATEST_VERIFY_MSG(threadId[1] != kThreadIdInvalid, "Barrier/Thread failure: Couldn't create thread.");

			if(threadId[0] != kThreadIdInvalid)
			{
				status = threads[0].WaitForEnd();
				EATEST_VERIFY_MSG(status != Thread::kStatusRunning, "Barrier/Thread failure.");
			}

			if(threadId[1] != kThreadIdInvalid)
			{
				status = threads[1].WaitForEnd();
				EATEST_VERIFY_MSG(status != Thread::kStatusRunning, "Barrier/Thread failure.");
			}
		}

	#endif

	return nErrorCount;
}










