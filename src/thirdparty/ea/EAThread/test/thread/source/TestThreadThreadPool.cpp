///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "TestThread.h"
#include <EATest/EATest.h>
#include <eathread/eathread_pool.h>
#include <eathread/eathread_atomic.h>
#include <stdlib.h>


using namespace EA::Thread;


const int kMaxConcurrentThreadCount = EATHREAD_MAX_CONCURRENT_THREAD_COUNT;


struct TPWorkData
{
   int mnWorkItem;
   TPWorkData(int nWorkItem) : mnWorkItem(nWorkItem) {}
};


static AtomicInt32 gWorkItemsCreated   = 0;
static AtomicInt32 gWorkItemsProcessed = 0;


static intptr_t WorkerFunction(void* pvWorkData)
{
   TPWorkData* pWorkData = (TPWorkData*)pvWorkData;

   ThreadId threadId = GetThreadId();
   EA::UnitTest::ReportVerbosity(1, "Work %4d starting for thread %s.\n", pWorkData->mnWorkItem, EAThreadThreadIdToString(threadId));

   EA::UnitTest::ThreadSleepRandom(200, 600);
   ++gWorkItemsProcessed;

   EA::UnitTest::ReportVerbosity(1, "Work %4d ending for thread   %s.\n", pWorkData->mnWorkItem, EAThreadThreadIdToString(threadId));

   delete pWorkData;

   return 0;
}


int TestThreadThreadPool()
{
	int nErrorCount(0);

	#if EA_THREADS_AVAILABLE
		{
			ThreadPoolParameters tpp;
			tpp.mnMinCount                = kMaxConcurrentThreadCount - 1;
			tpp.mnMaxCount                = kMaxConcurrentThreadCount - 1;
			tpp.mnInitialCount            = 0;

			tpp.mnIdleTimeoutMilliseconds = EA::Thread::kTimeoutNone;  // left in to test the usage of this kTimeout* defines.
			tpp.mnIdleTimeoutMilliseconds = 20000;
			

			ThreadPool threadPool(&tpp);
			int        nResult; 

			for(unsigned int i = 0; i < gTestLengthSeconds * 3; i++)
			{
				const int nWorkItem = (int)gWorkItemsCreated++;
				TPWorkData* const pWorkData = new TPWorkData(nWorkItem);

				EA::UnitTest::ReportVerbosity(1, "Work %4d created.\n", nWorkItem);
				nResult = threadPool.Begin(WorkerFunction, pWorkData, NULL, true);

				EATEST_VERIFY_MSG(nResult != ThreadPool::kResultError, "Thread pool failure in Begin.");

				EA::UnitTest::ThreadSleepRandom(300, 700);
				//Todo: If the pool task length gets too long, wait some more.
			}

			EA::UnitTest::ReportVerbosity(1, "Shutting down thread pool.\n");
			bool bShutdownResult = threadPool.Shutdown(ThreadPool::kJobWaitAll, GetThreadTime() + 60000);

			EATEST_VERIFY_MSG(bShutdownResult, "Thread pool failure in Shutdown (waiting for jobs to complete).");
		}

		EATEST_VERIFY_MSG(gWorkItemsCreated == gWorkItemsProcessed, "Thread pool failure: gWorkItemsCreated != gWorkItemsProcessed.");
	#endif

	return nErrorCount;
}











