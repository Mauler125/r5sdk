///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "TestThread.h"
#include <EATest/EATest.h>
#include <EAStdC/EABitTricks.h>
#include <EAStdC/EAStopwatch.h>
#include <EAStdC/EASprintf.h>
#include <EAStdC/EAString.h>
#include <EASTL/set.h>
#include <eathread/eathread_thread.h>
#include <eathread/eathread_sync.h>
#include <eathread/eathread_condition.h>

#ifdef EA_PLATFORM_LINUX
	#include <unistd.h>
#endif

#ifdef EA_PLATFORM_MICROSOFT
	EA_DISABLE_ALL_VC_WARNINGS()
	#include <Windows.h>
	EA_RESTORE_ALL_VC_WARNINGS()
#endif

#include <atomic>

using namespace EA::Thread;


const int           kMaxConcurrentThreadCount = EATHREAD_MAX_CONCURRENT_THREAD_COUNT;
static unsigned int sThreadTestTimeMS = 2000; // We potentially change this value below.
static AtomicInt32  sThreadCount = 0;
static AtomicInt32  sShouldGo = 0;


static intptr_t TestFunction1(void*)
{
	ThreadTime nTimeEnd = EA::Thread::GetThreadTime() + sThreadTestTimeMS;

	while(EA::Thread::GetThreadTime() < nTimeEnd)
		ThreadSleep();
	return 0;
}


static intptr_t TestFunction3(void*)
{
	return 0;
}


static intptr_t TestFunction4(void* arg)
{
	const intptr_t returnValue = (intptr_t)arg;

	EA::UnitTest::ThreadSleepRandom(0, 5);

	return returnValue;
}


#if !defined(EA_PLATFORM_MOBILE)
	static intptr_t TestFunction6(void* arg)
	{
		const intptr_t returnValue = (intptr_t)arg;

		sThreadCount++;

		while(sShouldGo == 0)
			ThreadSleep(10);

		EA::UnitTest::ThreadSleepRandom(3, 8);

		sThreadCount--;

		return returnValue;
	}
#endif


static intptr_t TestFunction7(void*)
{
	while(sShouldGo == 0)
		ThreadSleep(10);

	return 0;
}


static intptr_t TestFunction_GetThreadProcessor(void*)
{
	return (intptr_t)GetThreadProcessor();
}

static int findSetBitIndex(int requestedIndex, uint64_t availableAffinityMask)
{
	int correctedIndex = -1;
	int64_t mask = 1;
	do
	{
		if ((availableAffinityMask & mask) != 0)
			--requestedIndex;

		++correctedIndex;
		mask <<= 1;
	} while (requestedIndex >= 0);

	return correctedIndex;
}

static int calculateAvailableProcessorIndex(int index)
{
	using namespace EA::Thread;
	const auto affinityMask = GetAvailableCpuAffinityMask();
	const int availableCpuCount = EA::StdC::CountBits64(affinityMask);
	const int properIndex = findSetBitIndex(index % availableCpuCount, affinityMask);
	return properIndex;
}

static intptr_t TestFunction_SetAffinityAndWait(void* arg)
{
	const int requestedCore = *(static_cast<int*>(arg));
	const int coreIndex = calculateAvailableProcessorIndex(requestedCore);
	const ThreadAffinityMask affinityMask = UINT64_C(1) << coreIndex; 

	SetThreadAffinityMask(affinityMask);  

	while (GetThreadProcessor() != coreIndex)
	{
		SetThreadAffinityMask(affinityMask);  
		ThreadSleep(0);
	}

	return (intptr_t)coreIndex;
}

static intptr_t TestFunction_SetThreadProcessorAndWait(void* arg)
{
	const int requestedCore = *(static_cast<int*>(arg));
	const int coreIndex = calculateAvailableProcessorIndex(requestedCore);

	SetThreadProcessor(coreIndex);  

	while (GetThreadProcessor() != coreIndex)
	{
		SetThreadProcessor(coreIndex);  
		ThreadSleep(0);
	}

	return (intptr_t)coreIndex;
}

static intptr_t TestFunction_WaitForThreadMigration(void* arg)
{
	sThreadCount++;

	const int requestedCore = *(static_cast<int*>(arg));

	while (GetThreadProcessor() != requestedCore)
		ThreadSleep(0);

	sThreadCount--;

	return (intptr_t)requestedCore;
}

static intptr_t TestFunction13(void* arg)
{    
	ThreadSleep(10);    
	ThreadEnd(42);  // 42 is a magic number we will verify gets passed through the user.
	return 0; 
}

static intptr_t TestFunction3ExceptionWrapper(RunnableFunction defaultRunnableFunction, void* pContext)
{
	return defaultRunnableFunction(pContext);
}


class TestRunnable1 : public IRunnable
{
	intptr_t Run(void*)
	{
		const ThreadTime nTimeEnd = EA::Thread::GetThreadTime() + sThreadTestTimeMS;
		while (EA::Thread::GetThreadTime() < nTimeEnd)
			ThreadSleep();
		return 0;
	}
} gTestRunnable1;


class TestRunnable2 : public IRunnable
{
	intptr_t Run(void*)
	{
		const ThreadTime nTimeEnd = EA::Thread::GetThreadTime() + sThreadTestTimeMS;

		while(EA::Thread::GetThreadTime() < nTimeEnd)
		{
			ThreadSleep();
		}
		return 0;
	}
} gTestRunnable2;

static intptr_t TestRunnable3ExceptionWrapper(IRunnable* defaultRunnableFunction, void* pContext)
{
	return defaultRunnableFunction->Run(pContext);
}


class TestRunnable3 : public IRunnable
{
	intptr_t Run(void*)
	{
		ThreadSleep(500);
		return 0;
	}
} gTestRunnable3;

class TestRunnable4 : public IRunnable
{
	intptr_t Run(void* args)
	{
		// IRunnable object that returns the thread id that executed on.
		return TestFunction_WaitForThreadMigration(args);
	}
} gTestRunnable4;


int TestThreadAffinityMask()
{
	int nErrorCount = 0;
	const int TIMEOUT = 30000;
	const int MAX_ITERATIONS = 16;

	auto VERIFY_AFFINITY_RESULT = [&](const char* name, intptr_t in_result, int count)
	{
	#if EATHREAD_THREAD_AFFINITY_MASK_SUPPORTED
		const int nAvailableProcessors = EA::StdC::CountBits64(GetAvailableCpuAffinityMask());
		auto result = static_cast<int>(in_result);
		EATEST_VERIFY_F(
			result == calculateAvailableProcessorIndex(count),
			"Thread '%s' failure: SetAffinityMask not working properly. Thread ran on: %d/%d <=> Expected: %d\n", name,
			result, nAvailableProcessors, calculateAvailableProcessorIndex(count));
	#endif
	};

	int count = MAX_ITERATIONS;
	while(--count)
	{
		int properIndex = calculateAvailableProcessorIndex(count);
		// Test Thread Affinity Masks (thread parameters)
		Thread thread;
		ThreadParameters params;

		params.mnAffinityMask = INT64_C(1) << properIndex;
		params.mnProcessor = kProcessorAny;
		thread.Begin(TestFunction_GetThreadProcessor, NULL, &params);

		intptr_t result = 0;
		thread.WaitForEnd(GetThreadTime() + TIMEOUT, &result);

		VERIFY_AFFINITY_RESULT("Parameters", result, properIndex);
	}

	count = MAX_ITERATIONS;    
	while(--count)
	{
		int startIndex = calculateAvailableProcessorIndex(count + 1); // We explicitly want it to start on another core.
		int properIndex = calculateAvailableProcessorIndex(count);

		sThreadCount = 0;

		// Test Thread Affinity Masks (thread object)
		ThreadParameters params;
		params.mnProcessor = startIndex;

		Thread thread;
		thread.Begin(TestFunction_WaitForThreadMigration, &properIndex, &params);  // sleeps then grabs the current thread id. 

		#if defined(EA_PLATFORM_UNIX) || defined(EA_PLATFORM_NX)
			// spin while we wait for the thread to startup 
			while (sThreadCount == 0)
				ThreadSleep(1);
		#endif

		// after thread has started, set the requested affinity
		thread.SetAffinityMask(INT64_C(1) << properIndex);

		// wait for the thread to end
		intptr_t result = 0;
		thread.WaitForEnd(GetThreadTime() + TIMEOUT, &result);

		VERIFY_AFFINITY_RESULT("Object", result, properIndex);
	}    
	
	count = MAX_ITERATIONS;
	while(--count)
	{
		int properIndex = calculateAvailableProcessorIndex(count);

		// Test Thread Affinity Masks (global functions)       
		ThreadParameters params;
		params.mnProcessor = kProcessorAny;

		Thread thread;
		thread.Begin(TestFunction_SetAffinityAndWait, &properIndex, &params);

		intptr_t result = 0;
		thread.WaitForEnd(GetThreadTime() + TIMEOUT, &result);

		VERIFY_AFFINITY_RESULT("Global Function", result, properIndex);
	}

	count = MAX_ITERATIONS;    
	while(--count)
	{
		int properIndex = calculateAvailableProcessorIndex(count);

		// Test Thread Affinity Masks (thread parameters) - For IRunnable variant of the Thread::Begin function
		ThreadParameters params;
		params.mnProcessor = kProcessorAny;
		params.mnAffinityMask = INT64_C(1) << properIndex;
		params.mnProcessor = kProcessorAny;

		Thread thread;
		thread.Begin(&gTestRunnable4, &properIndex, &params);  // sleeps then grabs the current thread id.

		intptr_t result = 0;
		thread.WaitForEnd(GetThreadTime() + TIMEOUT, &result);

		VERIFY_AFFINITY_RESULT("IRunnable Thread Parameters", result, properIndex);
	}    

	// user regression of passing a -1 as an affinity mask.
	{
		// NOTE(rparolin): common user mistake. Ensure that we mask out invalid cpu flags and threads start up at least.
		int userRegressionMask = ~0u; 

		ThreadParameters params;
		params.mnProcessor = kProcessorAny;
		params.mnAffinityMask = userRegressionMask;
		params.mnProcessor = kProcessorAny;

		Thread thread;
		thread.Begin(TestFunction3, nullptr, &params);  
		thread.WaitForEnd(GetThreadTime() + TIMEOUT);
	}    

	return nErrorCount;
} 


int TestThreadPriorities()
{
	int nErrorCount = 0;

	if(!IsSuperUser())
	{
		EA::EAMain::Report("Skipping Thread Priority test because we don't have sufficient system priviliages.\n");
		return nErrorCount;
	}

	// Verify that thread priorities act as expected.
	// Threads with higher priorities should execute instead of or before threads of lower priorities.
	// On some platforms (e.g. Windows), lower priority threads do get some execution time, so we have to recognize that.

	// Create 20 threads of very high priority, 20 threads of high priority, and 20 threads of regular priority.
	// Start the 20 very high priority threads first. 
	// Wait a bit then start the other 40 threads.
	// Quit all the very high priority threads. 
	// Wait a bit, while having the 40 threads measure how much time they execute.
	// Quit the remaining 40 threads.
	// Verify that the 20 high priority threads executed much more than the regular threads.

	struct PriorityTestThread : public EA::Thread::IRunnable
	{
		EA::Thread::Thread           mThread;
		EA::Thread::ThreadParameters mParameters;
		EA::Thread::Semaphore        mSemaphore;
		char                         mThreadName[16];
		volatile uint64_t            mCounter = 0;
		volatile bool                mbShouldRun = true;
		char						 mPadd[EA_CACHE_LINE_SIZE];// make sure that these structures end up on different cachelines

		intptr_t Run(void*)
		{
			mSemaphore.Wait();

			while(mbShouldRun)
			{
				mCounter++;
				EAReadBarrier();
			}

			return 0;
		}
	};

	if((EA::Thread::GetProcessorCount() >= 4))
	{
		const int                kThreadCount = 4;
		PriorityTestThread       threadHighestPriority[kThreadCount];
		PriorityTestThread       threadRegularPriority[kThreadCount];
		PriorityTestThread       threadHighPriority[kThreadCount];
		EA::StdC::LimitStopwatch limitStopwatch(EA::StdC::Stopwatch::kUnitsSeconds);
		const EA::Thread::ThreadAffinityMask kCommonAffinityMask = 0xf; // first 4 cores only

		EA::Thread::ThreadParameters commonParams;
		commonParams.mbDisablePriorityBoost = true; // we can disable boosting if we want a better simulation of console-like behavior 
		commonParams.mnAffinityMask = kCommonAffinityMask;
		commonParams.mnProcessor = kProcessorAny;

#if defined(EA_PLATFORM_MICROSOFT)
		// Due to how windows thread priorities work we need to further increase the thread priority of the high threads
		// If this is not done the test will randomly have the regular priority have a higher count than the high threads. 
		const int kHigherPriorityDelta = kThreadPriorityMax - 2;
#else
		const int kHigherPriorityDelta = 0;
#endif

		const auto highestPriority = EA::Thread::kThreadPriorityDefault + 2 + kHigherPriorityDelta;
		const auto highPriority    = EA::Thread::kThreadPriorityDefault + 1 + kHigherPriorityDelta;
		const auto regularPriority = EA::Thread::kThreadPriorityDefault + 0;

		EA::Thread::SetThreadPriority(EA::Thread::kThreadPriorityDefault + 3);
		for (int i = 0; i < kThreadCount; i++)
		{
			{
				PriorityTestThread& highestThread = threadHighestPriority[i];
				highestThread.mParameters = commonParams;
				highestThread.mParameters.mnPriority = highestPriority;
				EA::StdC::Sprintf(highestThread.mThreadName, "Highest%d", i);
				highestThread.mParameters.mpName = highestThread.mThreadName;
				highestThread.mThread.Begin(&highestThread, NULL, &highestThread.mParameters);
			}
			{
				PriorityTestThread& regularThread = threadRegularPriority[i];
				regularThread.mParameters = commonParams;
				regularThread.mParameters.mnPriority = regularPriority;
				EA::StdC::Sprintf(regularThread.mThreadName, "Reg%d", i);
				regularThread.mParameters.mpName = regularThread.mThreadName;
				regularThread.mThread.Begin(&regularThread, NULL, &regularThread.mParameters);
			}
			{
				PriorityTestThread& highThread = threadHighPriority[i];
				highThread.mParameters = commonParams;
				highThread.mParameters.mnPriority = highPriority;
				EA::StdC::Sprintf(highThread.mThreadName, "High%d", i);
				highThread.mParameters.mpName = highThread.mThreadName;
				highThread.mThread.Begin(&highThread, NULL, &highThread.mParameters);
			}
		}

		limitStopwatch.SetTimeLimit(1, true); 
		while(!limitStopwatch.IsTimeUp())
			{ /* Do nothing. Don't even sleep, as some platform might not give us the CPU back. */ }

		for(int i = 0; i < kThreadCount; i++)
		{
			threadHighestPriority[i].mSemaphore.Post(1); 
			threadHighPriority[i].mSemaphore.Post(1); 
			threadRegularPriority[i].mSemaphore.Post(1);
		}

		limitStopwatch.SetTimeLimit(3, true); 
		while(!limitStopwatch.IsTimeUp())
			{ /* Do nothing. Don't even sleep, as some platform might not give us the CPU back. */ }

		for(int i = 0; i < kThreadCount; i++)
			threadHighestPriority[i].mbShouldRun = false;
		EAWriteBarrier();

		limitStopwatch.SetTimeLimit(3, true); 
		while(!limitStopwatch.IsTimeUp())
			{ /* Do nothing. Don't even sleep, as some platform might not give us the CPU back. */ }

		for(int i = 0; i < kThreadCount; i++)
		{
			threadHighPriority[i].mbShouldRun = false;
			threadRegularPriority[i].mbShouldRun = false;
		}
		EAWriteBarrier();

		limitStopwatch.SetTimeLimit(3, true); 
		while(!limitStopwatch.IsTimeUp())
			{ /* Do nothing. Don't even sleep, as some platform might not give us the CPU back. */ }


		// Wait for the threads to end before continuing.
		for (int i = 0; i < kThreadCount; i++)
		{
			threadHighestPriority[i].mThread.WaitForEnd();
			threadHighPriority[i].mThread.WaitForEnd();
			threadRegularPriority[i].mThread.WaitForEnd();
		}

		EA::Thread::SetThreadPriority(EA::Thread::kThreadPriorityDefault);

		EAReadWriteBarrier();

		// Tally the thread priority counters
		uint64_t highPriorityCount    = 0;
		uint64_t regularPriorityCount = 0;
		for(int i = 0; i < kThreadCount; i++)
			highPriorityCount += threadHighPriority[i].mCounter;
		for(int i = 0; i < kThreadCount; i++)
			regularPriorityCount += threadRegularPriority[i].mCounter;

	#ifndef EA_PLATFORM_LINUX
		// TODO(rparolin): This unreliable on various linux distros. Requires investigation.
		EATEST_VERIFY_F(highPriorityCount > regularPriorityCount,
						"Priority execution failure: highPriorityCount: %I64u, regularPriorityCount: %I64u",
						highPriorityCount, regularPriorityCount);
	#endif
	}
	
	return nErrorCount;
}

int TestSetThreadProcessConstants()
{
	int nErrorCount = 0;

	// testing a user reported regression of negative value shifts
	for(auto k : { kProcessorDefault, kProcessorAny })
	{
		Thread t;

		t.Begin([](void* param) -> intptr_t
				{
					int kConstant = *(int*)param;
					SetThreadProcessor(kConstant);
					return 0;
				}, &k);
		t.WaitForEnd();
	}

	return nErrorCount;
}

int TestNullThreadNames()
{
	int nErrorCount = 0;

	{
		ThreadParameters threadParams;
		threadParams.mpName = nullptr;

		Thread t;
		t.Begin([](void*) -> intptr_t { return 0; }, NULL, &threadParams);
		t.WaitForEnd();
	}

	return nErrorCount;
}

int TestLambdaThreads()
{
	int nErrorCount = 0;

	{ // test rvalue
		int foo = 0;
		MakeThread([&]
				   {
					   EATEST_VERIFY(foo == 0);
					   foo = 42;
				   })
			.WaitForEnd();
		EATEST_VERIFY(foo == 42);
	}

	{ // test lvalue
		int foo = 0;

		auto callme = [&]
		{
			EATEST_VERIFY(foo == 0);
			foo = 42;
		};

		MakeThread(callme).WaitForEnd();
		EATEST_VERIFY(foo == 42);
	}

	{ // test thread parameters
		const char* MY_THREAD_NAME = "my thread name";
		ThreadParameters params;
		params.mpName = MY_THREAD_NAME;

		MakeThread(
			[&] { EATEST_VERIFY(strncmp(MY_THREAD_NAME, GetThreadName(), EATHREAD_NAME_SIZE) == 0); }, params)
				.WaitForEnd();
	}

	return nErrorCount;
}

int TestThreadDynamicData()
{
	int nErrorCount = 0;

	const int kOverflowDynamicDataCount = 256; // Must be greater than EA::Thread::kMaxThreadDynamicDataCount.

	for(int i = 0; i < kOverflowDynamicDataCount; i++)
	{
		EA::Thread::ThreadId id = 0;
		EA::Thread::SysThreadId sysId = 0;

		MakeThread([&]
				{
					id = EA::Thread::GetThreadId();
					sysId = EA::Thread::GetSysThreadId();
				})
		.WaitForEnd();

		EATEST_VERIFY(FindThreadDynamicData(id) == nullptr);
		EATEST_VERIFY(FindThreadDynamicData(sysId) == nullptr);
	}

	return nErrorCount;
}

int TestSetThreadProcessor()
{
	int nErrorCount = 0;

	const int TIMEOUT = 30000;

	auto VERIFY_THREAD_PROCESSOR_RESULT = [&](const char* name, intptr_t in_result, int count)
	{
	#if EATHREAD_THREAD_AFFINITY_MASK_SUPPORTED
		const int nAvailableProcessors = EA::StdC::CountBits64(EA::Thread::GetAvailableCpuAffinityMask());
		auto result = static_cast<int>(in_result);
		EATEST_VERIFY_F(
			result == calculateAvailableProcessorIndex(count),
			"Thread '%s' failure: SetAffinityMask not working properly. Thread ran on: %d/%d <=> Expected: %d\n", name,
			result, nAvailableProcessors, calculateAvailableProcessorIndex(count));
	#endif
	};

	int count = EA::StdC::CountBits64(EA::Thread::GetAvailableCpuAffinityMask());
	while(--count)
	{
		int correctIndex = calculateAvailableProcessorIndex(count);
		Thread thread;
		thread.Begin(TestFunction_SetThreadProcessorAndWait, &correctIndex, nullptr);  // sleeps then grabs the current thread id. 

		// wait for the thread to end
		intptr_t result = 0;
		thread.WaitForEnd(GetThreadTime() + TIMEOUT, &result);

		VERIFY_THREAD_PROCESSOR_RESULT("SetThreadProcessor", result, correctIndex);
	}    

	return nErrorCount;
}

int TestSetProcessor()
{
	int nErrorCount = 0;

	const int TIMEOUT = 30000;
	const int MAX_ITERATIONS = 16;

	auto VERIFY_SET_PROCESSOR_RESULT = [&](const char* name, intptr_t in_result, int count)
	{
	#if EATHREAD_THREAD_AFFINITY_MASK_SUPPORTED
		const int nAvailableProcessors = EA::StdC::CountBits64(EA::Thread::GetAvailableCpuAffinityMask());
		auto result = static_cast<int>(in_result);
		EATEST_VERIFY_F(
			result == calculateAvailableProcessorIndex(count),
			"Thread '%s' failure: SetProcessor not working properly. Thread ran on: %d/%d <=> Expected: %d\n", name,
			result, nAvailableProcessors, calculateAvailableProcessorIndex(count));
	#endif
	};


	int count = MAX_ITERATIONS;    
	while(--count)
	{
		int startIndex = calculateAvailableProcessorIndex(count + 1); // We explicitly want it to start on another core.
		int properIndex = calculateAvailableProcessorIndex(count);

		sThreadCount = 0;

		// Test Thread Affinity Masks (thread object)
		ThreadParameters params;
		params.mnProcessor = startIndex;
		
		Thread thread;
		thread.Begin(TestFunction_WaitForThreadMigration, &properIndex, &params);  // sleeps then grabs the current thread id. 

		#if defined(EA_PLATFORM_UNIX) || defined(EA_PLATFORM_NX)
			// spin while we wait for the thread to startup 
			while (sThreadCount == 0)
				ThreadSleep(0);
		#endif

		// after thread has started, set the requested affinity
		thread.SetProcessor(properIndex);

		// wait for the thread to end
		intptr_t result = 0;
		thread.WaitForEnd(GetThreadTime() + TIMEOUT, &result);

		VERIFY_SET_PROCESSOR_RESULT("SetProcessor", result, properIndex);
	}    

	return nErrorCount;
}

int TestSetThreadProcessorByThreadParams()
{
	// Exercise EA::Thread::GetThreadId, EA::Thread::GetSysThreadId, EAThreadGetUniqueId, SetThreadProcessor, GetThreadProcessor.

	// Create and start N threads paused.
	// Release all threads to run at once.
	// Have each of the threads record its EAThreadGetUniqueId value and exit.
	// Verify that there were no collisions in the recorded id values.
	int nErrorCount = 0;

	struct IdTestThread : public EA::Thread::IRunnable
	{
		EA::Thread::Thread         mThread;                 // The Thread object.
		EA::Thread::Semaphore      mSemaphore;              // Used to pause the thread after it starts.
		EA::Thread::ThreadUniqueId mUniqueId;               // The EAThreadUniqueId that this thread gets assigned by the OS.
		EA::Thread::ThreadId       mThreadId;               // The EAThreadUniqueId that this thread gets assigned by the OS.
		EA::Thread::SysThreadId    mSysThreadId;            // The EAThreadUniqueId that this thread gets assigned by the OS.
		int                        mAssignedProcessorId;    // The processor id that we ask the OS to run this thread on.
		int                        mProcessorId;            // The processor id that this thread gets assigned by the OS. Should equal mAssignedProcessorId.

		IdTestThread() : mThread(), mSemaphore(0), mUniqueId(), mThreadId(), mSysThreadId(), mAssignedProcessorId(), mProcessorId() {}
		IdTestThread(const IdTestThread&){} // Avoid compiler warnings.
		void operator=(const IdTestThread&){}     // Avoid compiler warnings.

		intptr_t Run(void*)
		{
			mSemaphore.Wait();
			EAThreadGetUniqueId(mUniqueId);
			mThreadId    = EA::Thread::GetThreadId();
			mSysThreadId = EA::Thread::GetSysThreadId();
			mProcessorId = EA::Thread::GetThreadProcessor();
			EAWriteBarrier();
			return 0;
		}
	};

	#if defined(EA_PLATFORM_DESKTOP)
	  const int kThreadCount = 100;
	#elif (EA_PLATFORM_WORD_SIZE == 8)
	  const int kThreadCount = 50;
	#else
	  const int kThreadCount = 16;
	#endif

	IdTestThread                thread[kThreadCount];
	ThreadParameters            threadParams;
	EA::UnitTest::RandGenT<int> random(EA::UnitTest::GetRandSeed());
	const int                   processorCount = EA::StdC::CountBits64(EA::Thread::GetAvailableCpuAffinityMask());

	for(int i = 0; i < kThreadCount; i++)
	{
		threadParams.mnProcessor = calculateAvailableProcessorIndex(random(processorCount));
		threadParams.mpName = "IdTest";
		thread[i].mAssignedProcessorId = threadParams.mnProcessor;
		thread[i].mThread.Begin(&thread[i], NULL, &threadParams);
	}

	EA::UnitTest::ThreadSleep(1000);

	for(int i = 0; i < kThreadCount; i++)
		thread[i].mSemaphore.Post(1); 

	EA::UnitTest::ThreadSleep(1000);

	for(int i = 0; i < kThreadCount; i++)
		thread[i].mThread.WaitForEnd();

	EAReadBarrier();

	EA::Thread::ThreadUniqueId uniqueIdArray[kThreadCount];
	EA::Thread::ThreadId       idArray[kThreadCount];
	EA::Thread::SysThreadId    sysIdArray[kThreadCount];

	// Problem: We don't have an EAThreadEqual(const ThreadId&, const ThreadId&) function, but could use one.
	// If we had such a thing, then we wouldn't need the odd code below and could probably use an eastl::set.
	for(int i = 0; i < kThreadCount; i++)
	{
		memset(&uniqueIdArray[i], 0, sizeof(EA::Thread::ThreadUniqueId));
		memset(&idArray[i],       0, sizeof(EA::Thread::ThreadId));
		memset(&sysIdArray[i],    0, sizeof(EA::Thread::SysThreadId));
	}

	for(int i = 0; i < kThreadCount; i++)
	{
		for(int j = 0; j < i; j++)
			EATEST_VERIFY(memcmp(&uniqueIdArray[j], &thread[i].mUniqueId, sizeof(EA::Thread::ThreadUniqueId)) != 0);
		uniqueIdArray[i] = thread[i].mUniqueId;

		for(int j = 0; j < i; j++)
			EATEST_VERIFY(memcmp(&idArray[j], &thread[i].mThreadId, sizeof(EA::Thread::ThreadId)) != 0);
		idArray[i] = thread[i].mThreadId;

		for(int j = 0; j < i; j++)
			EATEST_VERIFY(memcmp(&sysIdArray[j], &thread[i].mSysThreadId, sizeof(EA::Thread::SysThreadId)) != 0);
		sysIdArray[i] = thread[i].mSysThreadId;

		#if defined(EA_PLATFORM_CONSOLE) && defined(EA_PLATFORM_MICROSOFT)
			EATEST_VERIFY_F(thread[i].mProcessorId == thread[i].mAssignedProcessorId, 
							 "    Error: Thread assigned to run on processor %d, found to be running on processor %d.", 
							 thread[i].mAssignedProcessorId, thread[i].mProcessorId);
		#endif
	}
	
	return nErrorCount;
}


int TestThreadDisablePriorityBoost()
{
	int nErrorCount = 0;

#if defined(EA_PLATFORM_WINDOWS) || defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_XBSX)
	{
		Thread thread;
		ThreadParameters params;

		auto priorityBoostTester = [&](BOOL expectedDisablePriorityBoost)
		{
			thread.Begin(
			    [](void* pInExpectedDisablePriorityBoost) -> intptr_t
			    {
				    BOOL* pExpectedDisablePriorityBoost = (BOOL*)pInExpectedDisablePriorityBoost;
				    int nErrorCount = 0;
				    PBOOL pDisablePriorityBoost = nullptr;

				    auto result = GetThreadPriorityBoost(GetCurrentThread(), pDisablePriorityBoost);
				    EATEST_VERIFY_MSG(result != 0, "GetThreadPriorityBoost failed\n");
				    EATEST_VERIFY_MSG((result != 0) && *pDisablePriorityBoost == *pExpectedDisablePriorityBoost,
				                      "Thread Priority Boost was not disabled\n");

				    return nErrorCount;
				},
			    &expectedDisablePriorityBoost, &params);

			intptr_t threadErrorCount = 0;
			thread.WaitForEnd(GetThreadTime() + 30000, &threadErrorCount);
			nErrorCount += (int)threadErrorCount;
		};


		params.mbDisablePriorityBoost = true;
		priorityBoostTester(TRUE);

		params.mbDisablePriorityBoost = false;
		priorityBoostTester(FALSE);
	}
#endif

	return nErrorCount;
}


int TestThreadParameters()
{ // Test ThreadParameters
	int nErrorCount = 0;
	Thread::Status   status;
	const int        kThreadCount(kMaxConcurrentThreadCount - 1);
	ThreadId         threadId[kThreadCount];
	Thread           thread[kThreadCount];
	int              i;
	ThreadParameters threadParameters;

	const int nOriginalPriority = GetThreadPriority();

	// Set our thread priority to match that of the threads we will be creating below.
	SetThreadPriority(kThreadPriorityDefault + 1);

	for(i = 0; i < kThreadCount; i++)
	{
		status = thread[i].GetStatus();
		EATEST_VERIFY_MSG(status == Thread::kStatusNone, "Thread failure: Thread should have kStatusNone (2).\n");

		// ThreadParameters
		threadParameters.mnStackSize = 32768;  
		threadParameters.mnPriority  = kThreadPriorityDefault + 1;
		threadParameters.mpName      = "abcdefghijklmnopqrstuvwxyz"; // Make an overly large name.

		threadId[i] = thread[i].Begin(TestFunction1, NULL, &threadParameters);

		EATEST_VERIFY_MSG(threadId[i] != (ThreadId)kThreadIdInvalid, "Thread failure: ThreadBegin failed.\n");

		// It turns out that you can't really do such a thing as set lower priority in native Linux, not with SCHED_OTHER, at least.
		#if ((!defined(EA_PLATFORM_UNIX) && !defined(__APPLE__)) || defined(__CYGWIN__)) && !EA_USE_CPP11_CONCURRENCY
			EATEST_VERIFY_MSG(thread[i].GetPriority() == threadParameters.mnPriority, "Thread failure: Thread Priority not set correctly (2).\n");
			 
		#endif

		if(i > 0)
			EATEST_VERIFY_MSG(threadId[i] != threadId[i-1], "Thread failure: Thread id collision (2).\n");
	}

	ThreadSleep(200);

	for(i = 0; i < kThreadCount; i++)
	{
		if(threadId[i] != kThreadIdInvalid)
			thread[i].SetName("0123456789012345678901234567890"); // Make an overly large name.
	}

	ThreadSleep(200);

	// It turns out that you can't really do such a thing as set lower priority in native Linux.
	#if ((!defined(EA_PLATFORM_UNIX) || defined(__CYGWIN__)) && !defined(__APPLE__) && !EA_USE_CPP11_CONCURRENCY)
		int nPriority;

		if(threadId[0] != kThreadIdInvalid)
		{
			nPriority = thread[0].GetPriority();

			thread[0].SetPriority(nPriority - 1);
			EATEST_VERIFY_MSG(thread[0].GetPriority() == nPriority - 1, "Thread failure: Thread Priority not set correctly (3.1).\n");

			thread[0].SetPriority(nPriority);
			EATEST_VERIFY_MSG(thread[0].GetPriority() == nPriority, "Thread failure: Thread Priority not set correctly (3.2).\n");
			 
		}
	#endif

	for(i = 0; i < kThreadCount; i++)
	{
		if(threadId[i] != kThreadIdInvalid)
			thread[i].WaitForEnd(GetThreadTime() + 30000);
	}

	SetThreadPriority(kThreadPriorityDefault);

	for(i = 0; i < kThreadCount; i++)
	{
		status = thread[i].GetStatus();

		if(threadId[i] != kThreadIdInvalid)
			EATEST_VERIFY_MSG(status == Thread::kStatusEnded, "Thread failure: Thread should have kStatusEnded (2).\n");
		else
			EATEST_VERIFY_MSG(status == Thread::kStatusNone, "Thread failure: Thread should have kStatusNone (2).\n");
	}
	
	SetThreadPriority(nOriginalPriority);
	return nErrorCount;
}

int TestThreadEnd()
{
	int nErrorCount(0);
	EA::Thread::Thread thread;
	Thread::Status   status;

	thread.Begin(TestFunction13); 
	ThreadSleep(20);
	
	intptr_t returncode;
	thread.WaitForEnd();
	status = thread.GetStatus(&returncode);
	
	EATEST_VERIFY_MSG(returncode == 42, "Thread return code failure:  Expected return code 42.");
	EATEST_VERIFY_MSG(status == Thread::kStatusEnded, "Thread failure: Thread should have kStatusEnded.\n");
 
	return nErrorCount;
}

EA_OPTIMIZE_OFF()
int TestThreadThread()
{
	int nErrorCount(0);

	sThreadTestTimeMS = (gTestLengthSeconds * 1000) / 2; // '/2' because this test doesn't need so much time.

	nErrorCount += TestSetThreadProcessor();
	nErrorCount += TestSetProcessor();

#if !defined(EA_PLATFORM_UNIX) && !defined(EA_PLATFORM_MOBILE)
	nErrorCount += TestThreadAffinityMask();
#endif


	nErrorCount += TestThreadEnd();

	{
		ThreadId threadId = GetThreadId();

		EATEST_VERIFY_MSG(threadId != kThreadIdInvalid, "GetThreadId failure.\n");

		SysThreadId sysThreadId = GetSysThreadId(threadId);

		EATEST_VERIFY_MSG(sysThreadId != kSysThreadIdInvalid, "GetSysThreadId failure.\n");

		#if (defined(EA_PLATFORM_MICROSOFT) || defined(EA_PLATFORM_UNIX) || defined(EA_PLATFORM_PS4)) && !EA_USE_CPP11_CONCURRENCY
			const void*    pStackBase = GetThreadStackBase();
			const void*    pStackTop  = &pStackBase;
			const intptr_t stackSize  = ((char*)pStackBase - (char*)pStackTop);

			// Verify that pStackBase is non-NULL and that the stack size is less than N MB.
			EATEST_VERIFY_MSG(pStackBase && (stackSize < 10485760), "GetThreadStackBase failure.\n");
			
		#endif

		// We disable this test for now on Kettle because although we have 7 cores available 
		// there is no guaranty the their ID are 0..6 and the system takes core ID 7
		#if !defined(EA_PLATFORM_PS4)

			int processorCount = GetProcessorCount();
			int processor      = GetThreadProcessor();
			
			// This isn't much of a test, but it at least exercizes the function.
			EATEST_VERIFY_F(processor < processorCount, "    Error: GetThreadProcessor [%d] >= GetProcessorCount [%d].\n", processor, processorCount);
		#endif

		// To do: Test this:
		// void SetThreadProcessor(int nProcessor);
	}

	{ // Test Current thread functionality
		const int nOriginalPriority = GetThreadPriority();
		int nPriority = kThreadPriorityDefault;

		EATEST_VERIFY_MSG(nPriority >= kThreadPriorityMin, "Thread priority failure (1).\n");
		EATEST_VERIFY_MSG(nPriority <= kThreadPriorityMax, "Thread priority failure (2).\n");

		// It turns out that you can't really do such a thing as set lower priority with most Unix threading subsystems. 
		// You can do so with Cygwin because it is just a pthreads API running on Windows OS/threading.
		// C++11 thread libraries also provide no means to set or query thread priority.
		#if (!defined(EA_PLATFORM_UNIX) || defined(__CYGWIN__)) && !EA_USE_CPP11_CONCURRENCY
			int  nPriority1;
			bool bResult;

			bResult = SetThreadPriority(nPriority);
			EATEST_VERIFY_MSG(bResult, "Thread priority failure (3).\n");

			bResult = SetThreadPriority(nPriority - 1);
			EATEST_VERIFY_MSG(bResult, "Thread priority failure (4).\n");

			nPriority1 = GetThreadPriority();
			EATEST_VERIFY_MSG(nPriority1 == nPriority - 1, "Thread priority failure (5).\n");

			bResult = SetThreadPriority(nPriority + 1);
			EATEST_VERIFY_MSG(bResult, "Thread priority failure (6).\n");

			nPriority1 = GetThreadPriority();
			EATEST_VERIFY_MSG(nPriority1 == nPriority + 1, "Thread priority failure (7).\n");

			bResult = SetThreadPriority(kThreadPriorityDefault);
			EATEST_VERIFY_MSG(bResult, "Thread priority failure (8).\n");

			nPriority1 = GetThreadPriority();
			EATEST_VERIFY_MSG(nPriority1 == kThreadPriorityDefault, "Thread priority failure (9).\n");
			 
		#endif

		SetThreadPriority(nOriginalPriority);
		ThreadSleep(kTimeoutImmediate);
		ThreadSleep(500);
	}

	#if defined(EA_PLATFORM_WINDOWS) && !EA_USE_CPP11_CONCURRENCY
	{ // Try to reproduce Windows problem with Thread::GetStatus returning kStatusEnded when it should return kStatusRunning.
		// On my current work machine (WinXP32, Single P4 CPU) this problem doesn't occur. But it might occur with others.
		Thread::Status status;
		Thread         threadBackground[8];
		Thread         thread;

		for(int i = 0; i < 8; i++)
			threadBackground[i].Begin(TestFunction1);

		EA::Thread::SetThreadPriority(kThreadPriorityDefault + 2);
		thread.Begin(TestFunction1);
		status = thread.GetStatus();
		EA::Thread::SetThreadPriority(kThreadPriorityDefault);

		EATEST_VERIFY_MSG(status == Thread::kStatusRunning, "Thread failure: Thread should have kStatusRunning.\n");

		thread.WaitForEnd();
	}
	#endif


	EA::Thread::Thread::SetGlobalRunnableFunctionUserWrapper(TestFunction3ExceptionWrapper);
	EA::Thread::Thread::SetGlobalRunnableClassUserWrapper(TestRunnable3ExceptionWrapper);

	{ // Test thread creation functionality.
		Thread::Status status;
		const int      kThreadCount(kMaxConcurrentThreadCount - 1);
		Thread         thread[kThreadCount];
		ThreadId       threadId[kThreadCount];
		int            i;

		for(i = 0; i < kThreadCount; i++)
		{
			status = thread[i].GetStatus();
			EATEST_VERIFY_MSG(status == Thread::kStatusNone, "Thread failure: Thread should have kStatusNone (1).\n");

			threadId[i] = thread[i].Begin(TestFunction1);
			EATEST_VERIFY_MSG(threadId[i] != kThreadIdInvalid, "Thread failure: ThreadBegin(TestFunction1) failed.\n");

			threadId[i] = thread[i].Begin(TestFunction3);
			EATEST_VERIFY_MSG(threadId[i] != kThreadIdInvalid, "Thread failure: ThreadBegin(TestFunction3) failed.\n");

			threadId[i] = thread[i].Begin(&gTestRunnable3);
			EATEST_VERIFY_MSG(threadId[i] != kThreadIdInvalid, "Thread failure: ThreadBegin(&gTestRunnable3) failed.\n");

			if(i > 0)
				EATEST_VERIFY_MSG(threadId[i] != threadId[i-1], "Thread failure: Thread id collision (1).\n");
		}

		// It turns out that you can't really do such a thing as set lower priority in native Linux.
		// C++11 threads also have no support for priorities
		#if (!defined(EA_PLATFORM_UNIX) || defined(__CYGWIN__)) && !EA_USE_CPP11_CONCURRENCY
			int nPriority;

			nPriority = thread[0].GetPriority();
			thread[0].SetPriority(nPriority - 1);

			nPriority = thread[0].GetPriority();
			thread[0].SetPriority(nPriority + 1);
		#endif

		ThreadSleep(200);

		for(i = 0; i < kThreadCount; i++)
		{
			if(threadId[i] != kThreadIdInvalid)
				thread[i].WaitForEnd(GetThreadTime() + 30000);
		}

		for(i = 0; i < kThreadCount; i++)
		{
			if(threadId[i] != kThreadIdInvalid)
			{
				status = thread[i].GetStatus();

				EATEST_VERIFY_MSG(status == Thread::kStatusEnded, "Thread failure: Thread should have kStatusEnded (1).\n");
			}
		}
	}


	nErrorCount += TestThreadParameters();

	{
		// Test if we can set and retrieve a custom thread name
		Thread::Status status;
		Thread         thread;
		ThreadId       threadId;
		const char*    threadName = "Test_Thread";
		const char*    defaultName = "DEFAULT";
		ThreadParameters threadParameters;

		threadParameters.mpName = defaultName;

		sThreadTestTimeMS = 10000;
		threadId = thread.Begin(TestFunction1, NULL, &threadParameters);

		EATEST_VERIFY_MSG(threadId != kThreadIdInvalid, "Thread failure: ThreadBegin(TestFunction1) failed.\n");
		EATEST_VERIFY_MSG(strncmp(defaultName, thread.GetName(), EATHREAD_NAME_SIZE) == 0, "Thread failure: GetName should return the name used when initializing the thread.");

		thread.SetName(threadName);

		EATEST_VERIFY_MSG(strncmp(threadName, thread.GetName(), EATHREAD_NAME_SIZE) == 0, "Thread failure: GetName should return the name set in SetName.");

		ThreadSleep(sThreadTestTimeMS + 1000);

		if(threadId != kThreadIdInvalid)
		{
			thread.WaitForEnd(GetThreadTime() + 30000);
			status = thread.GetStatus();
			EATEST_VERIFY_MSG(status == Thread::kStatusEnded, "Thread failure: Thread should have kStatusEnded (3).\n");
		}
	}

	// Test the free standing function variant of the GetName/SetName API
	{
		// Test if we can set and retrieve a custom thread name
		Thread::Status status;
		Thread         thread;
		ThreadId       threadId;
		const char*    threadName = "Test_Thread";
		const char*    defaultName = "DEFAULT";

		ThreadParameters threadParameters;
		threadParameters.mpName = defaultName;
		threadParameters.mnProcessor = EA::Thread::kProcessorAny;
		threadParameters.mnAffinityMask = EA::Thread::GetAvailableCpuAffinityMask();

		static volatile std::atomic<bool> sbThreadStarted;
		static volatile std::atomic<bool> sbThreadTestDone;
		sbThreadStarted = false;
		sbThreadTestDone = false;

		threadId = thread.Begin( [](void*) -> intptr_t
			{
				sbThreadStarted = true;
				while (!sbThreadTestDone)
					ThreadSleep();
				return 0;
			},
			NULL, &threadParameters);

		while(!sbThreadStarted) // Wait for thread to start up
			ThreadSleep();

		EATEST_VERIFY_MSG(threadId != kThreadIdInvalid, "Thread failure: ThreadBegin(TestFunction1) failed.\n");
		EATEST_VERIFY_MSG(strncmp(defaultName, GetThreadName(threadId), EATHREAD_NAME_SIZE) == 0, "Thread failure: GetName should return the name used when initializing the thread.");

		SetThreadName(threadId, threadName);
		EATEST_VERIFY_MSG(strncmp(threadName, GetThreadName(threadId), EATHREAD_NAME_SIZE) == 0, "Thread failure: GetName should return the name set in SetName.");

		sbThreadTestDone = true;  // signal that test is completed and the thread can shutdown 
		if(threadId != kThreadIdInvalid)
		{
			thread.WaitForEnd(GetThreadTime() + 30000);
			status = thread.GetStatus();
			EATEST_VERIFY_MSG(status == Thread::kStatusEnded, "Thread failure: Thread should have kStatusEnded (3).\n");
		}
	}


	{
		// Test the creation+destruction of many threads to make sure resources are recycled properly

		#if defined(EA_PLATFORM_DESKTOP)
		const int kThreadCount(200);
		#else
		const int kThreadCount(20);
		#endif
		Thread    thread;
		ThreadId  threadId;
		intptr_t  returnValue;

		EA::UnitTest::ReportVerbosity(1, "Creating many threads and then WaitForEnd()\n");

		for(int i = 0; i < kThreadCount; i++)
		{
			threadId = thread.Begin(TestFunction4, reinterpret_cast<void*>((uintptr_t)i));

			Thread::Status status = thread.GetStatus(&returnValue);
			if(status != Thread::kStatusEnded)
				thread.WaitForEnd(GetThreadTime() + 30000, &returnValue);
			EA::UnitTest::ReportVerbosity(1, "Thread ended.\n");

			EATEST_VERIFY_F(returnValue == i, "Thread failure: Thread return code is wrong (1). threadId: %s, status: %d, returnValue: %d\n", EAThreadThreadIdToString(threadId), (int)status, (int)returnValue);
			if(returnValue != i)
				EA::UnitTest::ReportVerbosity(1, "    Expected: %u, actual: %" PRId64 ".\n", (unsigned)i, (int64_t)returnValue);

			// Get the status again to make sure it returns the correct status.
			thread.GetStatus(&returnValue);
			EATEST_VERIFY_F(returnValue == i, "Thread failure: Thread return code is wrong (2). threadId: %s, status: %d, returnValue: %d\n", EAThreadThreadIdToString(threadId), (int)status, (int)returnValue);
			if(returnValue != i)
				EA::UnitTest::ReportVerbosity(1, "    Expected: %u, actual: %" PRId64 ".\n", (unsigned)i, (int64_t)returnValue);
		}

		
		EA::UnitTest::ReportVerbosity(1, "Creating many threads and then repeat GetStatus() until they finish\n");
		for(int i = 0; i < kThreadCount + 1; i++)
		{
			// Windows will get stuck in infinite loop if return value is 259 (STILL_ACTIVE)
			if(i == 259)
				 ++i;           

			threadId = thread.Begin(TestFunction4, reinterpret_cast<void*>((uintptr_t)i));

			const ThreadTime nGiveUpTime = EA::Thread::GetThreadTime() + 20000; // Give up after N milliseconds.

			// Test to see if GetStatus() will recycle system resource properly
			while(thread.GetStatus(&returnValue) != Thread::kStatusEnded)
			{
				ThreadSleep(100);

				if(EA::Thread::GetThreadTime() > nGiveUpTime)
				{
					EA::UnitTest::ReportVerbosity(1, "Thread failure: GetStatus failed.\n");
					break;
				}
			}

			EATEST_VERIFY_MSG(returnValue == i, "Thread failure: Thread return code is wrong (3).\n");
			if(returnValue != i)
				EA::UnitTest::ReportVerbosity(1, "    Expected: %u, actual: %" PRId64 ".\n", (unsigned)i, (int64_t)returnValue);

			// Get the status again to make sure it returns the correct status.
			thread.GetStatus(&returnValue);
			EATEST_VERIFY_MSG(returnValue == i, "Thread failure: Thread return code is wrong (4).\n");
			if(returnValue != i)
				EA::UnitTest::ReportVerbosity(1, "    Expected: %u, actual: %" PRId64 ".\n", (unsigned)i, (int64_t)returnValue);

			// See if calling WaitForEnd() now will result in a crash
			thread.WaitForEnd(GetThreadTime() + 30000, &returnValue);
			EATEST_VERIFY_MSG(returnValue == i, "Thread failure: Thread return code is wrong (5).\n");
		}
	}

	{
		// Test the creation of many threads

		#if defined(EA_PLATFORM_DESKTOP)
			Thread::Status status;
			const int      kThreadCount(96);
			Thread         thread[kThreadCount];
			ThreadId       threadId[kThreadCount];
			int            i;

			sThreadTestTimeMS = 10000;

			for(i = 0; i < kThreadCount; i++)
			{
				threadId[i] = thread[i].Begin(TestFunction1);
				EATEST_VERIFY_MSG(threadId[i] != kThreadIdInvalid, "Thread failure: ThreadBegin(TestFunction1) failed.\n");
			}

			ThreadSleep(sThreadTestTimeMS + 1000);

			for(i = 0; i < kThreadCount; i++)
			{
				if(threadId[i] != kThreadIdInvalid)
					thread[i].WaitForEnd(GetThreadTime() + 30000);
			}

			for(i = 0; i < kThreadCount; i++)
			{
				if(threadId[i] != kThreadIdInvalid)
				{
					status = thread[i].GetStatus();
					EATEST_VERIFY_MSG(status == Thread::kStatusEnded, "Thread failure: Thread should have kStatusEnded (3).\n");
				}
			}
		#endif
	}

	{
		// Regression of Thread dtor behaviour - the dtor should not wait for created threads
		// We reuse the atomic shouldgo to figure out what is really happening setting it to 0
		// initially and then incrementing it when the thread callback completes, allowing
		// us to detect whether the dtor completed before the thread callback.
		sShouldGo = 0;
		{
			// Create a scope for our thread object
			Thread thread;
			
			// Get our thread going. It will sleep immediately for 2 seconds and then increment sShouldGo
			ThreadId threadId = thread.Begin(TestFunction7);
			EATEST_VERIFY_MSG(threadId != kThreadIdInvalid, "Thread failure: thread.Begin(TestFunction7) failed.\n");

			// Now we exit our scope while our thread in theory still has a second before it completes
		} // NOTE(rparolin): If your test hangs here, its most likely due to a semantic change in the thread object that is waiting for threads to complete.

		// Signal to the thread it is allowed to complete.
		sShouldGo = 1;
	}


	{
		int nOriginalPriority = EA::Thread::GetThreadPriority();
		EA::Thread::SetThreadPriority(kThreadPriorityDefault);

		// Tests setting and getting thread priority while the thread is active.
		Thread::Status  status;
		Thread          thread;
		ThreadId        threadId;
		sShouldGo       = 0;

		threadId = thread.Begin(TestFunction7);
		EATEST_VERIFY_MSG(threadId != kThreadIdInvalid, "Thread failure: ThreadBegin(TestFunction7) failed.\n");
		
		// It turns out that you can't really do such a thing as set lower priority in native Linux.
		#if ((!defined(EA_PLATFORM_LINUX) && !defined(__APPLE__) && !defined(EA_PLATFORM_BSD)) || defined(__CYGWIN__)) && !EA_USE_CPP11_CONCURRENCY
			int nPriority = thread.GetPriority();

			thread.SetPriority(nPriority - 1);
			if(EATEST_VERIFY_MSG(thread.GetPriority() == nPriority - 1, "Thread failure: Thread Priority not set correctly (2).\n"))
				 nErrorCount++;

			thread.SetPriority(nPriority);
			EATEST_VERIFY_MSG(thread.GetPriority() == nPriority, "Thread failure: Thread Priority not set correctly (3).\n");
		#endif

		sShouldGo = 1;
		thread.WaitForEnd(GetThreadTime() + 30000);

		status = thread.GetStatus();
		EATEST_VERIFY_MSG(status == Thread::kStatusEnded, "Thread failure: Thread should have kStatusEnded.\n");
		EA::Thread::SetThreadPriority(nOriginalPriority);
	}

	nErrorCount += TestThreadDynamicData();
	nErrorCount += TestThreadPriorities();
	nErrorCount += TestSetThreadProcessorByThreadParams();
	nErrorCount += TestSetThreadProcessConstants();
	nErrorCount += TestNullThreadNames();
	nErrorCount += TestLambdaThreads();

	{
		// Test SetDefaultProcessor
		Thread::SetDefaultProcessor(kProcessorAny);
	}


	#if !defined(EA_PLATFORM_MOBILE)
		// This test does not execute correctly on Android. The 'while(sThreadCount
		// < kThreadCount)' loop sometimes hangs indefinitely because the spawned
		// threads exit often before the loop continues, so the spawned
		// thread count never gets up to kThreadCount. I guess the inner loop
		// should be testing addedThreadCount too. (?) But, just disable for now,
		// since I guess it's working on other platforms.
		{
			// Test very many threads starting and quitting, recycling
			// The PS3 code had problems with leaking threads in this case.
			// Create a bunch of threads.
			// Every N ms quit one of them and create a new one.

			// This test tends to be quite taxing on system resources, so cut it back for 
			// certain platforms. To do: use EATest's platform speed metrics.
		ThreadParameters params;
		const int kThreadCount(48); // This needs to be greater than the eathread_thread.cpp kMaxThreadDynamicDataCount value.
		#ifdef EA_PLATFORM_NX
			const int kTotalThreadsToRun(64);

			const auto nAllCoresAffinityMask = ((INT64_C(1) << EA::Thread::GetProcessorCount()) - 1);
			const auto nAllCoresButMainAffinityMask = nAllCoresAffinityMask & INT64_C(0XFFFFFFFFFFFFFE);

			params.mnAffinityMask = nAllCoresButMainAffinityMask;
			params.mnProcessor = kProcessorAny;
		#else
			const int kTotalThreadsToRun(500);
		#endif

			Thread    thread;
			ThreadId  threadId;
			int       addedThreadCount = 0;
			int       i = 0;

			sThreadCount = 0;
			sShouldGo    = 0;

			// Create threads.
			for(i = 0; i < kThreadCount; i++)
			{
				threadId = thread.Begin(TestFunction6, reinterpret_cast<void*>((uintptr_t)i), &params);
				EATEST_VERIFY(threadId != kThreadIdInvalid);
			}

			// Wait until they are all created and started.
			while(sThreadCount < kThreadCount)
				ThreadSleep(500);

			// Let them run and exit as they go.
			sShouldGo = 1;

			// Add new threads as existing ones leave.
			while(addedThreadCount < kTotalThreadsToRun)
			{
				if(sThreadCount < kThreadCount)
				{
					threadId = thread.Begin(TestFunction6, reinterpret_cast<void*>((uintptr_t)i++), &params);

					EATEST_VERIFY(threadId != kThreadIdInvalid);

					addedThreadCount++;
					
					#if defined(EA_PLATFORM_DESKTOP)
						// The created threads will not get any time slices on weaker platforms.
						// thus making the test create threads until the system is out of resources
						ThreadSleep(kTimeoutYield);

						// Sometimes it will never exit this loop.. because it will leave faster then it is made
						if(addedThreadCount >= kTotalThreadsToRun)
							break;
					#endif
				}
				else
					ThreadSleep(10);
			}

			// Wait until they have all exited.
			while(sThreadCount != 0) // While there are threads...
				ThreadSleep(500);

			// On some platforms it seems that thread entry/exit is so slow that the thread count might not necessarily
			// reflect the actual number of threads running.  This was causing an issue where the function was
			// exiting before all threads completed, so I added this wait.  It may be worth considering 
			// keeping track of all created threads in an array, and then using a thread-join operation here
			// instead
			ThreadSleep(1000);
		}

	#endif

	return nErrorCount;
}









