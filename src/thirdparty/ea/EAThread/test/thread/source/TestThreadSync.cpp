///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "TestThread.h"
#include <EATest/EATest.h>
#include <eathread/eathread_sync.h>
#include <eathread/eathread.h>


int TestThreadSync()
{
	using namespace EA::Thread;

	int nErrorCount(0);

	EAReadBarrier();
	EAWriteBarrier();
	EAReadWriteBarrier();
	EACompilerMemoryBarrier();

	return nErrorCount;
}



