///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "TestThread.h"
#include <EATest/EATest.h>
#include <eathread/eathread.h>
#include <eathread/eathread_storage.h>
#include <eathread/eathread_thread.h>

using namespace EA::Thread;

const int kMaxConcurrentThreadCount = EATHREAD_MAX_CONCURRENT_THREAD_COUNT;


///////////////////////////////////////////////////////////////////////////////
// EA_THREAD_LOCAL tests
//
#ifdef EA_THREAD_LOCAL

	// Test declaration of basic int.
	EA_THREAD_LOCAL int gTLI = 0;

	// Test declaration of static int.
	static EA_THREAD_LOCAL int sTLI = 0;

	// Test declaration of struct.
	struct ThreadLocalData
	{
		int x;
	};

	EA_THREAD_LOCAL ThreadLocalData gTDL;

#endif




struct TLSWorkData
{
	AtomicInt32         mShouldBegin;
	AtomicInt32         mShouldEnd;
	AtomicInt32         mnErrorCount;
	ThreadLocalStorage* mpTLS;

	TLSWorkData()
		: mShouldBegin(0), mShouldEnd(0), mnErrorCount(0), mpTLS(NULL) {}

	// define copy ctor and assignment operator
	// so the compiler does define them intrisically
	TLSWorkData(const TLSWorkData& rhs);               // copy constructor
	TLSWorkData& operator=(const TLSWorkData& rhs);    // assignment operator
};



static intptr_t TLSTestThreadFunction(void* pvWorkData)
{
	TLSWorkData* pWorkData = (TLSWorkData*)pvWorkData;
	void*        pValue;
	bool         bResult;
	size_t       i = 1;
	int          nErrorCount = 0;

	while(pWorkData->mShouldBegin.GetValue() == 0) // Spin until we get the should-begin signal. We could also use a Semaphore for this, but our use here is very simple and not performance-oriented.
		EA::Thread::ThreadSleep(100);

	pValue = pWorkData->mpTLS->GetValue();
	EATEST_VERIFY_MSG(pValue == NULL, "ThreadLocalStorage failure.");

	do{
		bResult = pWorkData->mpTLS->SetValue((void*)i);
		EATEST_VERIFY_MSG(bResult, "ThreadLocalStorage failure.");

		pValue = pWorkData->mpTLS->GetValue();
		EATEST_VERIFY_MSG(pValue == (void*)i, "ThreadLocalStorage failure.");

		i++;

		ThreadCooperativeYield(); // Used by cooperative threading platforms.
	}while(pWorkData->mShouldEnd.GetValue() == 0);

	pValue = pWorkData->mpTLS->GetValue();
	EATEST_VERIFY_MSG(pValue != NULL, "ThreadLocalStorage failure.");

	pWorkData->mnErrorCount += nErrorCount;

	return 0;
}


static int TestThreadStorageSingle()
{
	int nErrorCount(0);

	{  // Re-use test
		void* pValue;
		ThreadLocalStorage tlsUnused;

		{ // Give this its own scope.
			ThreadLocalStorage tls0;

			pValue = tls0.GetValue();
			EATEST_VERIFY_MSG(pValue == NULL, "ThreadLocalStorage failure.");

			tls0.SetValue((void*)1);
		}

		ThreadLocalStorage tls1;

		pValue = tls1.GetValue();
		EATEST_VERIFY_MSG(pValue == NULL, "ThreadLocalStorage failure.");
	}


	{  // Single-threaded test
		#ifdef EA_THREAD_LOCAL
			// EA_THREAD_LOCAL tests
			gTLI   = 1;
			sTLI   = 1;
			gTDL.x = 1;

			if((gTLI + sTLI + gTDL.x) == 100000) // Prevent compiler warnings due to non-usage.
				EA::UnitTest::ReportVerbosity(1, "EA_THREAD_LOCAL: %d %d %d\n", gTLI, sTLI, gTDL.x);
		#endif

		// Test ThreadLocalStorage
		ThreadLocalStorage tlsA;
		ThreadLocalStorage tlsB;
		ThreadLocalStorage tlsC;
		void* pValue;
		void* pNULL = (void*)0;
		void* p1    = (void*)1;
		void* p2    = (void*)2;
		bool  bResult;

		pValue = tlsA.GetValue();
		EATEST_VERIFY_MSG(pValue == pNULL, "ThreadLocalStorage failure.");

		pValue = tlsB.GetValue();
		EATEST_VERIFY_MSG(pValue == pNULL, "ThreadLocalStorage failure.");

		pValue = tlsC.GetValue();
		EATEST_VERIFY_MSG(pValue == pNULL, "ThreadLocalStorage failure.");

		bResult = tlsA.SetValue(pNULL);
		EATEST_VERIFY_MSG(bResult, "ThreadLocalStorage failure.");

		pValue = tlsA.GetValue();
		EATEST_VERIFY_MSG(pValue == pNULL, "ThreadLocalStorage failure.");

		bResult = tlsA.SetValue(p1);
		EATEST_VERIFY_MSG(bResult, "ThreadLocalStorage failure.");

		pValue = tlsA.GetValue();
		EATEST_VERIFY_MSG(pValue == p1, "ThreadLocalStorage failure.");

		bResult = tlsA.SetValue(pNULL);
		EATEST_VERIFY_MSG(bResult, "ThreadLocalStorage failure.");

		pValue = tlsA.GetValue();
		EATEST_VERIFY_MSG(pValue == pNULL, "ThreadLocalStorage failure.");

		bResult = tlsA.SetValue(p2);
		EATEST_VERIFY_MSG(bResult, "ThreadLocalStorage failure.");

		pValue = tlsA.GetValue();
		EATEST_VERIFY_MSG(pValue == p2, "ThreadLocalStorage failure.");
	}

	return nErrorCount;
}


static int TestThreadStorageMultiple()
{
	int nErrorCount(0);

	const int      kThreadCount(kMaxConcurrentThreadCount - 1);
	Thread         thread[kThreadCount];
	ThreadId       threadId[kThreadCount];
	Thread::Status status;
	int            i;
	TLSWorkData    workData;

	for(i = 0; i < kThreadCount; i++)
	{
		threadId[i] = thread[i].Begin(TLSTestThreadFunction, &workData);
		EATEST_VERIFY_MSG(threadId[i] != kThreadIdInvalid, "Thread failure: Couldn't create thread.");
	}

	workData.mpTLS = new ThreadLocalStorage;
	workData.mShouldBegin.SetValue(1);

	EA::UnitTest::ThreadSleepRandom(2000, 2000);

	workData.mShouldEnd.SetValue(1);

	EA::UnitTest::ThreadSleepRandom(1000, 1000);

	for(i = 0; i < kThreadCount; i++)
	{
		if(threadId[i] != kThreadIdInvalid)
		{
			status = thread[i].WaitForEnd(GetThreadTime() + 30000);
			EATEST_VERIFY_MSG(status == Thread::kStatusEnded, "Thread failure: Thread(s) didn't end.");
		}
	}

	delete workData.mpTLS;
	workData.mpTLS = NULL;

	nErrorCount += (int)workData.mnErrorCount;

	return nErrorCount;
}


int TestThreadStorage()
{
	int nErrorCount(0);

	nErrorCount += TestThreadStorageSingle();

	#if EA_THREADS_AVAILABLE
		// Call this twice, to make sure recyling of TLS works properly.
		nErrorCount += TestThreadStorageMultiple();
		nErrorCount += TestThreadStorageMultiple();
	#endif

	return nErrorCount;
}



