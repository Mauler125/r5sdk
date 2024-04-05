///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "TestThread.h"
#include <EATest/EATest.h>
#include <eathread/eathread_condition.h>
#include <eathread/eathread_thread.h>
#include <eathread/eathread_mutex.h>
#include <eathread/eathread_list.h>
#include <eathread/eathread_sync.h>
#include <stdlib.h>
#include <time.h>


using namespace EA::Thread;


///////////////////////////////////////////////////////////////////////////////
// EATHREAD_INTERPROCESS_CONDITION_SUPPORTED
//
#ifndef EATHREAD_INTERPROCESS_CONDITION_SUPPORTED
	#if defined(EA_PLATFORM_MICROSOFT) || defined(EA_PLATFORM_LINUX)
		#define EATHREAD_INTERPROCESS_CONDITION_SUPPORTED 1
	#else
		#define EATHREAD_INTERPROCESS_CONDITION_SUPPORTED 0
	#endif
#endif
 

const int kMaxConcurrentThreadCount = EATHREAD_MAX_CONCURRENT_THREAD_COUNT;


struct TMWorkData
{
	volatile bool                mbProducersShouldQuit;
	volatile bool                mbConsumersShouldQuit;
	EA::Thread::simple_list<int> mJobList;
	Condition                    mCondition;
	Mutex                        mMutex;
	int                          mnLastJobID;
	int                          mnConditionTimeout;
	AtomicInt32                  mnTotalJobsCreated;
	AtomicInt32                  mnTotalJobsCompleted;

	TMWorkData( const ConditionParameters* pCondParams ) : mbProducersShouldQuit(false), mbConsumersShouldQuit(false), mCondition( pCondParams ), 
				   mMutex(NULL, true), mnLastJobID(0), mnConditionTimeout(60000), mnTotalJobsCreated(0), mnTotalJobsCompleted(0)
	{
		// Empty
	}

	// define copy ctor and assignment operator
	// so the compiler does define them intrisically
	TMWorkData(const TMWorkData& rhs);               // copy constructor
	TMWorkData& operator=(const TMWorkData& rhs);    // assignment operator
};


static intptr_t ProducerFunction(void* pvWorkData)
{
	int         nErrorCount = 0;
	TMWorkData* pWorkData   = (TMWorkData*)pvWorkData;
	ThreadId    threadId    = GetThreadId();

	EA::UnitTest::ReportVerbosity(1, "Condition producer test function created: %s\n", EAThreadThreadIdToString(threadId));

	EAReadWriteBarrier();

	while(!pWorkData->mbProducersShouldQuit)
	{
		EA::UnitTest::ThreadSleepRandom(100, 200);
		pWorkData->mMutex.Lock();

		for(int i(0), iEnd(rand() % 3); i < iEnd; i++)
		{
			const int nJob(++pWorkData->mnLastJobID);
			pWorkData->mJobList.push_back(nJob);
			++pWorkData->mnTotalJobsCreated;
			EA::UnitTest::ReportVerbosity(1, "Job %d created by %s.\n", nJob, EAThreadThreadIdToString(threadId));
			ThreadCooperativeYield(); // Used by cooperative threading platforms.
		}

		pWorkData->mMutex.Unlock();
		pWorkData->mCondition.Signal(false);
		ThreadCooperativeYield(); // Used by cooperative threading platforms.
	}

	EA::UnitTest::ReportVerbosity(1, "Producer exiting: %s.\n", EAThreadThreadIdToString(threadId));

	return nErrorCount;
}

static intptr_t ProducerFunction_DoesNotSignal(void* pvWorkData)
{
	int         nErrorCount = 0;
	TMWorkData* pWorkData   = (TMWorkData*)pvWorkData;
	ThreadId    threadId    = GetThreadId();
	EA_UNUSED(pWorkData);

	EA::UnitTest::ReportVerbosity(1, "Condition producer (does not signal) test function created: %s\n", EAThreadThreadIdToString(threadId));

	// Intentionally  do nothing here.  We are testing the conditional variable time out code path by
	// ensuring we do not signal the Consumer that any work has been added into the queue for them
	// to consume therefor explicitly causing a condition variable timeout.

	EA::UnitTest::ReportVerbosity(1, "Producer (does not signal) exiting: %s.\n", EAThreadThreadIdToString(threadId));

	return nErrorCount;
}

static intptr_t ConsumerFunction(void* pvWorkData)
{
	int         nErrorCount = 0;
	TMWorkData* pWorkData   = (TMWorkData*)pvWorkData;
	ThreadId    threadId    = GetThreadId();

	EA::UnitTest::ReportVerbosity(1, "Condition producer test function created: %s\n", EAThreadThreadIdToString(threadId));

	pWorkData->mMutex.Lock();

	do{
		if(!pWorkData->mJobList.empty())
		{
			const int nJob = pWorkData->mJobList.front();
			pWorkData->mJobList.pop_front();
			pWorkData->mMutex.Unlock();

			ThreadCooperativeYield(); // Used by cooperative threading platforms.

			// Here we would actually do the job, but printing 'job done' is enough in itself.
			++pWorkData->mnTotalJobsCompleted;
			EA::UnitTest::ReportVerbosity(1, "Job %d done by %s.\n", nJob, EAThreadThreadIdToString(threadId));

			pWorkData->mMutex.Lock();
		}
		else
		{
			const ThreadTime timeoutAbsolute = GetThreadTime() + pWorkData->mnConditionTimeout;
			const Condition::Result result = pWorkData->mCondition.Wait(&pWorkData->mMutex, timeoutAbsolute);
			if((result != Condition::kResultOK) && pWorkData->mJobList.empty())
				break;
		}
	}while(!pWorkData->mbConsumersShouldQuit || !pWorkData->mJobList.empty());

	pWorkData->mMutex.Unlock();

	EA::UnitTest::ReportVerbosity(1, "Consumer exiting: %s.\n", EAThreadThreadIdToString(threadId));

	return nErrorCount;
}


int TestThreadCondition()
{
	int nErrorCount(0);

	{ // ctor tests
		// We test various combinations of Mutex ctor and ConditionParameters.
		// ConditionParameters(bool bIntraProcess = true, const char* pName = NULL);
		// Condition(const ConditionParameters* pConditionParameters = NULL, bool bDefaultParameters = true);

		ConditionParameters cp1(true, NULL);
		ConditionParameters cp2(true, "EATcp2");

	  #if EATHREAD_INTERPROCESS_CONDITION_SUPPORTED
		ConditionParameters cp3(false, NULL);
		ConditionParameters cp4(false, "EATcp4");
	  #else
		ConditionParameters cp3(true, NULL);
		ConditionParameters cp4(true, "EATcp4");
	  #endif

		// Create separate scopes below because some platforms are so  
		// limited that they can't create all of them at once.
		{
			Condition cond1(&cp1, false);
			Condition cond2(&cp2, false);
			Condition cond3(&cp3, false);

			cond1.Signal();
			cond2.Signal();
			cond3.Signal();
		}
		{
			Condition cond4(&cp4, false);
			Condition cond5(NULL, true);
			Condition cond6(NULL, false);
			cond6.Init(&cp1);

			cond4.Signal();
			cond5.Signal();
			cond6.Signal();
		}
	}


	#if EA_THREADS_AVAILABLE
		{
			// test producer/consumer wait condition with intra-process condition
			{
				ConditionParameters exlusiveConditionParams( true, NULL );
				TMWorkData workData( &exlusiveConditionParams );
				Thread::Status status;
				int i;
	
				const int kThreadCount(kMaxConcurrentThreadCount - 1);
				Thread    threadProducer[kThreadCount];
				ThreadId  threadIdProducer[kThreadCount];
				Thread    threadConsumer[kThreadCount];
				ThreadId  threadIdConsumer[kThreadCount];
	
				// Create producers and consumers.
				for(i = 0; i < kThreadCount; i++)
				{
					threadIdProducer[i] = threadProducer[i].Begin(ProducerFunction, &workData);
					EATEST_VERIFY_MSG(threadIdProducer[i] != kThreadIdInvalid, "Condition/Thread failure: Thread creation failed.");
	
					threadIdConsumer[i] = threadConsumer[i].Begin(ConsumerFunction, &workData);
					EATEST_VERIFY_MSG(threadIdConsumer[i] != kThreadIdInvalid, "Condition/Thread failure: Thread creation failed.");
				}
	
				EA::UnitTest::ThreadSleepRandom(gTestLengthSeconds * 1000, gTestLengthSeconds * 1000);
	
				// Wait for producers to quit.
				workData.mbProducersShouldQuit = true;
				for(i = 0; i < kThreadCount; i++)
				{
					if(threadIdProducer[i] != kThreadIdInvalid)
					{
						status = threadProducer[i].WaitForEnd(GetThreadTime() + 30000);
						EATEST_VERIFY_MSG(status != Thread::kStatusRunning, "Condition/Thread failure: Wait for producer end failed.");
					}
				}
	
				EA::UnitTest::ThreadSleepRandom(2000, 2000);
	
				// Wait for consumers to quit.
				workData.mbConsumersShouldQuit = true;
				workData.mCondition.Signal(true);
				for(i = 0; i < kThreadCount; i++)
				{
					if(threadIdConsumer[i] != kThreadIdInvalid)
					{
						status = threadConsumer[i].WaitForEnd(GetThreadTime() + 30000);
						EATEST_VERIFY_MSG(status != Thread::kStatusRunning, "Condition/Thread failure: Wait for consumer end failed.");
					}
				}
	
				EATEST_VERIFY_MSG(workData.mnTotalJobsCreated == workData.mnTotalJobsCompleted, "Condition failure: Not all consumer work was processed.");
			}

			// test single producer/ single consumer wait condition with inter-process condition
			#if /*EATHREAD_INTERPROCESS_CONDITION_SUPPORTED*/ 0 // Disabled because this code fails on most platforms.
			{
				ConditionParameters sharedConditionParams( false, NULL );
				TMWorkData     workData( &sharedConditionParams ); // Inter-process.
				Thread::Status status;
				Thread         threadProducer;
				Thread         threadConsumer;
	
				ThreadId threadIdProducer = threadProducer.Begin(ProducerFunction, &workData);
				EATEST_VERIFY_MSG(threadIdProducer != kThreadIdInvalid, "Condition/Thread failure: Thread creation failed.");
	
				ThreadId threadIdConsumer = threadConsumer.Begin(ConsumerFunction, &workData);
				EATEST_VERIFY_MSG(threadIdConsumer != kThreadIdInvalid, "Condition/Thread failure: Thread creation failed.");
				
				EA::UnitTest::ThreadSleepRandom(gTestLengthSeconds * 1000, gTestLengthSeconds * 1000);
	
				// Wait for producer to quit.
				workData.mbProducersShouldQuit = true;

				if(threadIdProducer != kThreadIdInvalid)
				{
					status = threadProducer.WaitForEnd(GetThreadTime() + 30000);
					EATEST_VERIFY_MSG(status != Thread::kStatusRunning, "Condition/Thread failure: Wait for producer end failed.");
				}

				EA::UnitTest::ThreadSleepRandom(2000, 2000);
	
				// Wait for consumers to quit.
				workData.mbConsumersShouldQuit = true;
				workData.mCondition.Signal(true);
				if(threadIdConsumer != kThreadIdInvalid)
				{
					status = threadConsumer.WaitForEnd(GetThreadTime() + 30000);
					EATEST_VERIFY_MSG(status != Thread::kStatusRunning, "Condition/Thread failure: Wait for consumer end failed.");
				}
	
				EATEST_VERIFY_MSG(workData.mnTotalJobsCreated == workData.mnTotalJobsCompleted, "Condition failure: Not all consumer work was processed.");
			}
			#endif
			
			// Test conditional variable timeout explicitly by not sending a signal.
			{
				//::EA::EAMain::SetVerbosity(5);
				ConditionParameters sharedConditionParams( true, NULL );
				TMWorkData     workData( &sharedConditionParams ); // Inter-process.
				workData.mnConditionTimeout = 3000;  // timeout value has to be less than thread timeout value below.
				Thread::Status status;
				Thread         threadProducer;
				Thread         threadConsumer;
	
				ThreadId threadIdProducer = threadProducer.Begin(ProducerFunction_DoesNotSignal, &workData);
				EATEST_VERIFY_MSG(threadIdProducer != kThreadIdInvalid, "Condition/Thread failure: Thread creation failed.");
	
				ThreadId threadIdConsumer = threadConsumer.Begin(ConsumerFunction, &workData);
				EATEST_VERIFY_MSG(threadIdConsumer != kThreadIdInvalid, "Condition/Thread failure: Thread creation failed.");

				EA::UnitTest::ThreadSleepRandom(gTestLengthSeconds * 1000, gTestLengthSeconds * 1000);
				
				// Wait for producer to quit.
				if(threadIdProducer != kThreadIdInvalid)
				{
					status = threadProducer.WaitForEnd(GetThreadTime() + 30000);
					EATEST_VERIFY_MSG(status != Thread::kStatusRunning, "Condition/Thread failure: Wait for producer end failed.");
				}

				EA::UnitTest::ThreadSleepRandom(2000, 2000);

				// Wait for consumers to quit.
				workData.mbConsumersShouldQuit = true;
				if(threadIdConsumer != kThreadIdInvalid)
				{
					status = threadConsumer.WaitForEnd(GetThreadTime() + 30000);
					EATEST_VERIFY_MSG(status != Thread::kStatusRunning, "Condition/Thread failure: Wait for consumer end failed.");
				}
			}
		}
	#endif

	return nErrorCount;
}



