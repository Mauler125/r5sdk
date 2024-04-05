///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "TestThread.h"
#include <EATest/EATest.h>
#include <eathread/eathread_spinlock.h>


using namespace EA::Thread;


int TestThreadSpinLock()
{
	int nErrorCount(0);

	{ // SpinLock -- Basic single-threaded test.

		SpinLock spinLock;

		EATEST_VERIFY_MSG(!spinLock.IsLocked(), "SpinLock failure");

		spinLock.Lock();
		EATEST_VERIFY_MSG(spinLock.IsLocked(), "SpinLock failure");

		EATEST_VERIFY_MSG(!spinLock.TryLock(), "SpinLock failure");

		spinLock.Unlock();
		EATEST_VERIFY_MSG(!spinLock.IsLocked(), "SpinLock failure");

		EATEST_VERIFY_MSG(spinLock.TryLock(), "SpinLock failure");
		EATEST_VERIFY_MSG(spinLock.IsLocked(), "SpinLock failure");

		spinLock.Unlock();
		EATEST_VERIFY_MSG(!spinLock.IsLocked(), "SpinLock failure");
	}


	{ // AutoSpinLock -- Basic single-threaded test.

		SpinLock spinLock;

		EATEST_VERIFY_MSG(!spinLock.IsLocked(), "AutoSpinLock failure");

		{  //Special scope just for the AutoSpinLock
			AutoSpinLock autoSpinLock(spinLock);

			EATEST_VERIFY_MSG(spinLock.IsLocked(), "AutoSpinLock failure");
		}

		EATEST_VERIFY_MSG(!spinLock.IsLocked(), "AutoSpinLock failure");
	}


	#if EA_THREADS_AVAILABLE
		{  // Multithreaded test
	  
			// Implement this when we get the thread class working.
			// gTestLengthSeconds;
		}
	#endif

	return nErrorCount;
}











