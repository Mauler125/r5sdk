///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#include "TestThread.h"
#include <eathread/eathread.h>
#include <eathread/eathread_callstack.h>
#include <EATest/EATest.h>
#include <EAStdC/EAString.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Prevents false positive memory leaks on GCC/Clang platforms
#if defined(EA_COMPILER_GNUC) || defined(EA_COMPILER_CLANG)
	#define EA_MEMORY_GCC_USE_FINALIZE
#endif

#ifndef EA_OPENSOURCE
	#include <MemoryMan/MemoryMan.inl>
	#include <MemoryMan/CoreAllocator.inl>
	#include <coreallocator/icoreallocator_interface.h>
#endif


#ifdef EA_OPENSOURCE
void* operator new[](size_t size, const char* /*pName*/, int /*flags*/,
					 unsigned /*debugFlags*/, const char* /*file*/, int /*line*/)
{
	return operator new[](size);
}

void* operator new[](size_t size, size_t /*alignment*/, size_t /*alignmentOffset*/, const char* /*pName*/,
					 int /*flags*/, unsigned /*debugFlags*/, const char* /*file*/, int /*line*/)
{
	return operator new[](size);
}
#endif


#ifdef EA_DLL
	#include <MemoryMan/MemoryManDLL.h>
#endif

#if defined(EA_PLATFORM_MICROSOFT)
	EA_DISABLE_ALL_VC_WARNINGS()
	#include <Windows.h>
	EA_RESTORE_ALL_VC_WARNINGS()
#endif

#if defined(EA_COMPILER_MSVC) && defined(EA_PLATFORM_MICROSOFT)
	EA_DISABLE_ALL_VC_WARNINGS()
	#include <crtdbg.h>
	EA_RESTORE_ALL_VC_WARNINGS()
#endif

#include <EAMain/EAEntryPointMain.inl>
#include <EATest/EASTLVsnprintf.inl>
#include <EAMain/EAEntryPointMain.inl>


///////////////////////////////////////////////////////////////////////////////
// gTestLengthSeconds
//
unsigned int gTestLengthSeconds = 2;



///////////////////////////////////////////////////////////////////////////////
// EAThreadFailure
//
// This is called by EAThread's assert failure function.
//
static void EAThreadFailure(const char* pMessage, void* /*pContext*/)
{
	EA::UnitTest::IncrementGlobalErrorCount(1);
	EA::UnitTest::Report("Thread test failure (EAThread assert): %s\n", pMessage);
}



///////////////////////////////////////////////////////////////////////////////
// TestThreadMisc
//
// To do: Move this to its own file.
//
#if defined(EA_PLATFORM_APPLE)
	#include <eathread/apple/eathread_callstack_apple.h>
#endif

#if defined(EA_PLATFORM_POSIX)
#include <unistd.h>
#endif

bool IsSuperUser()
{
#if defined(EA_PLATFORM_POSIX) && !defined(EA_PLATFORM_SONY) && !defined(EA_PLATFORM_NX)  // PS4 is a POSIX machine but doesn't implement 'getuid'.
	// http://pubs.opengroup.org/onlinepubs/009695399/functions/geteuid.html 
	// http://pubs.opengroup.org/onlinepubs/009695399/functions/getuid.html
	uid_t uid = getuid(), euid = geteuid();
	return (uid == 0 || euid == 0);
#else
	return true;
#endif
}



int TestThreadGetThreadTimeMin()
{
	int nErrorCount(0);
#if defined(EA_PLATFORM_MICROSOFT) || defined(EA_PLATFORM_PS4)
	EATEST_VERIFY_MSG(EA::Thread::GetThreadTime() >= EATHREAD_MIN_ABSOLUTE_TIME, "Reported GetThreadTime absolute time is less than EATHREAD_MIN_ABSOLUTE_TIME. You are going to have a bad time.");
#endif
	return nErrorCount;
}

int TestThreadMisc()
{
	int nErrorCount = 0;
	// this is here because its intended to be the first test run. Since it depends on tick count since title start.
	nErrorCount += TestThreadGetThreadTimeMin();

	#if defined(EA_PLATFORM_APPLE) && EATHREAD_APPLE_GETMODULEINFO_ENABLED
		if(!IsSuperUser())
		{
			EA::EAMain::Report("Skipping GetModuleInfoApple test because we don't have sufficient system privileges.\n");
			return nErrorCount;
		}

		EA::Thread::ModuleInfoApple moduleInfoApple[15];
		size_t n = EA::Thread::GetModuleInfoApple(moduleInfoApple, EAArrayCount(moduleInfoApple), NULL, true);

		#if defined(EA_PLATFORM_OSX)
			EATEST_VERIFY(n > 0);
			for(size_t i = 0; i < eastl::min(n, EAArrayCount(moduleInfoApple)); i++)
				EA::UnitTest::Report("%s\n", moduleInfoApple[i].mPath);
		#else
			EA_UNUSED(n);
		#endif
	#endif
	
	return nErrorCount;
}



///////////////////////////////////////////////////////////////////////////////
// EAMain
//
int EAMain(int argc, char** argv)
{
	using namespace EA::Thread;
	using namespace EA::UnitTest;

	int nErrorCount(0);

	EA::EAMain::PlatformStartup();

	gTestLengthSeconds = (unsigned int)(3 * EA::UnitTest::GetSystemSpeed());
	if(gTestLengthSeconds == 0)
		gTestLengthSeconds = 1;

	// Set EAThread to route its errors to our own error reporting function.
	EA::Thread::SetAssertionFailureFunction(EAThreadFailure, NULL);

	#if defined(EA_DLL) && defined(EA_MEMORY_ENABLED) && EA_MEMORY_ENABLED
		EA::Allocator::InitSharedDllAllocator();
	#endif

	//Set EAThread to use the Default Allocator to keep track of our memory usage
#ifndef EA_OPENSOURCE
	EA::Thread::SetAllocator(EA::Allocator::ICoreAllocator::GetDefaultAllocator());
#endif

	// Print ThreadId for this primary thread.
	const ThreadId threadId = GetThreadId();
	ReportVerbosity(1, "Primary thread ThreadId: %s\n", EAThreadThreadIdToString(threadId));

	// Print SysThreadId for this primary thread.
	const SysThreadId sysThreadId = GetSysThreadId(threadId);
	ReportVerbosity(1, "Primary thread SysThreadId: %s\n", EAThreadSysThreadIdToString(sysThreadId));

	// Print thread priority for this primary thread.
	const int nPriority = EA::Thread::GetThreadPriority();
	ReportVerbosity(1, "Primary thread priority: %d\n", nPriority);

	const int nProcessorCount = EA::Thread::GetProcessorCount();
	ReportVerbosity(1, "Currently active virtual processor count: %d\n", nProcessorCount);

	#if defined(EA_PLATFORM_MICROSOFT) && !EA_POSIX_THREADS_AVAILABLE && !EA_USE_CPP11_CONCURRENCY
		const DWORD dwCurrentThreadId = GetCurrentThreadId(); // This is a system OS call.
		EATEST_VERIFY_F(sysThreadId == dwCurrentThreadId, "GetSysThreadId failed. SysThreadId = %u, sys thread id = %u.\n", (unsigned)sysThreadId, (unsigned)dwCurrentThreadId);
	#endif

	// Add the tests
	TestApplication testSuite("EAThread Unit Tests", argc, argv);

	#if defined(EA_PLATFORM_NX)
		// We are enabling round-robin scheduling on the Nintendo NX by setting the main thread to the lowest thread
		// priority.  The pthread interfaces inherits the main threads priority when creating new threads so this
		// ensures all worker threads spawned in tests will be assigned equal time on cores.  This is a documented
		// feature of the NX thread scheduler. The default behaviour of the scheduler is to not give up their core to
		// threads of equal priority which causes live locks in our tests.
		//
		// Reference:
		// https://developer.nintendo.com/html/online-docs/nx-en/g1kr9vj6-en/document.html?doc=Packages/SDK/NintendoSDK/Documents/Package/contents/Pages/Page_83955697.html
		EA::Thread::SetThreadPriority(EA::Thread::kThreadPriorityMin);
	#endif

	testSuite.AddTest("Atomic",            TestThreadAtomic);
	testSuite.AddTest("Barrier",           TestThreadBarrier);
	testSuite.AddTest("Callstack",         TestThreadCallstack);
	testSuite.AddTest("Condition",         TestThreadCondition);
	testSuite.AddTest("EnumerateThreads",  TestEnumerateThreads);
	testSuite.AddTest("Futex",             TestThreadFutex);
	testSuite.AddTest("Misc",              TestThreadMisc);
	testSuite.AddTest("Mutex",             TestThreadMutex);
	testSuite.AddTest("RWMutex",           TestThreadRWMutex);
	testSuite.AddTest("RWSemaphore",       TestThreadRWSemaLock);
	testSuite.AddTest("RWSpinLock",        TestThreadRWSpinLock);
	testSuite.AddTest("Semaphore",         TestThreadSemaphore);
	testSuite.AddTest("SmartPtr",          TestThreadSmartPtr);
	testSuite.AddTest("SpinLock",          TestThreadSpinLock);
	testSuite.AddTest("Storage",           TestThreadStorage);
	testSuite.AddTest("Sync",              TestThreadSync);
	testSuite.AddTest("TestCpuAvailableAffinityMask", TestCpuAvailableAffinityMask);
	testSuite.AddTest("Thread",            TestThreadThread);
	testSuite.AddTest("ThreadPool",        TestThreadThreadPool);

	nErrorCount += testSuite.Run();

#ifndef EA_USE_CPP11_CONCURRENCY
	// Verify the converted a EA::Thread::ThreadId to a EA::Thread::SysThreadId id matches this thread context id.
	const ThreadId convertedThreadId = GetThreadId(sysThreadId);
	EATEST_VERIFY_F(threadId == convertedThreadId , "GetThreadId failed to convert SysThreadId. ThreadId = %s, converted thread id = %s.\n", EAThreadThreadIdToString(threadId),  EAThreadThreadIdToString(convertedThreadId));
#endif

	EA::EAMain::PlatformShutdown(nErrorCount);

	return nErrorCount;
}









