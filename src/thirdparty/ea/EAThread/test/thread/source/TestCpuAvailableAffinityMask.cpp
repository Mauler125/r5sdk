///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include <EATest/EATest.h>
#include "TestThread.h"
#include <eathread/eathread_thread.h>
#include <eathread/eathread_semaphore.h>
#include <eathread/eathread_sync.h>
#include <eathread/eathread_atomic.h>
#include <EAStdC/EAString.h>

#if defined(EA_PLATFORM_STADIA)
	EATHREADLIB_API bool isUint(const char* buffer);
	EATHREADLIB_API EA::Thread::ThreadAffinityMask parseSet(char const* buffer, size_t len);
	EATHREADLIB_API bool readLine(const char* fileName, char* buffer, size_t len);
	EATHREADLIB_API bool getProcessCpuSetCGroup(char* cgroup, size_t len);
	EATHREADLIB_API EA::Thread::ThreadAffinityMask getInstanceCpuSet();
	EATHREADLIB_API EA::Thread::ThreadAffinityMask initValidCpuAffinityMask();
#endif


int TestCpuAvailableAffinityMaskBasic()
{
	int nErrorCount = 0;

#if defined(EA_PLATFORM_STADIA)
	{ // Test isUint()
		static const char* isUintTest1 = "1";
		static const char* isUintTest2 = "wrong";
		static const char* isUintTest3 = "    9";
		static const char* isUintTest4 = "    ";
		static const char* isUintTest5 = "";

		EATEST_VERIFY(isUint(isUintTest1));
		EATEST_VERIFY(!isUint(isUintTest2));
		EATEST_VERIFY(isUint(isUintTest3));
		EATEST_VERIFY(!isUint(isUintTest4));
		EATEST_VERIFY(!isUint(isUintTest5));
	}

	{ // Test parseSet()
		char const* parseSetTest1 = "0-6";
		char const* parseSetTest2 = "0, 2-6, 8";
		char const* parseSetTest3 = "wrong";

		EA::Thread::ThreadAffinityMask parseSetVerify1 = 0x7f;
		EA::Thread::ThreadAffinityMask parseSetVerify2 = 0x17d;
		EA::Thread::ThreadAffinityMask parseSetVerify3 = 0;

		EA::Thread::ThreadAffinityMask parseSetResult1 = parseSet(parseSetTest1, EA::StdC::Strlen(parseSetTest1));
		EA::Thread::ThreadAffinityMask parseSetResult2 = parseSet(parseSetTest2, EA::StdC::Strlen(parseSetTest2));
		EA::Thread::ThreadAffinityMask parseSetResult3 = parseSet(parseSetTest3, EA::StdC::Strlen(parseSetTest3));
		EATEST_VERIFY(parseSetVerify1 == parseSetResult1);
		EATEST_VERIFY(parseSetVerify2 == parseSetResult2);
		EATEST_VERIFY(parseSetVerify3 == parseSetResult3);
	}

	{ // Test getProcessCpuSetCGroup()
		EATEST_VERIFY(!getProcessCpuSetCGroup(nullptr, 0));

		char cgroup[40];
		EATEST_VERIFY(getProcessCpuSetCGroup(cgroup, sizeof(cgroup)));
		EATEST_VERIFY(EA::StdC::Strlen(cgroup) != 0);
	}
	
	{ // Test getInstanceCpuSet()
		EA::Thread::ThreadAffinityMask instanceCpuSet = getInstanceCpuSet();
		EATEST_VERIFY(instanceCpuSet != 0);
	}

	{ // Test initValidCpuAffinityMask()
		EA::Thread::ThreadAffinityMask cpuAffinityMask = initValidCpuAffinityMask();
		EATEST_VERIFY(cpuAffinityMask != 0);
	}
#endif

	return nErrorCount;
}


int TestCpuAvailableAffinityMask()
{
	int nErrorCount = 0;
	
	nErrorCount += TestCpuAvailableAffinityMaskBasic();

	{ // Test GetAvailableCpuAffinityMask()
		EA::Thread::ThreadAffinityMask availableCpuAffinityMask = EA::Thread::GetAvailableCpuAffinityMask();

		EATEST_VERIFY(availableCpuAffinityMask != 0);
		EATEST_VERIFY(availableCpuAffinityMask != EA::Thread::kThreadAffinityMaskAny);
	}

	return nErrorCount;
}
