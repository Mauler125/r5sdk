///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "TestThreadInterprocess.h"
#include <EATest/EATest.h>
#include <EAMain/EAMain.h>
#include <eathread/eathread.h>
#include <EAStdC/EAString.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <EAMain/EAEntryPointMain.inl>


///////////////////////////////////////////////////////////////////////////////
// Globals
//
unsigned int gTestThreadCount   =  4;
unsigned int gTestLengthSeconds = 10;


///////////////////////////////////////////////////////////////////////////////
// EAThreadFailure
//
// This is called by EAThread's assert failure function.
//
static void EAThreadFailure(const char* pMessage, void* /*pContext*/)
{
	EA::UnitTest::Report("Thread test failure (EAThread assert): %s\n", pMessage);
}



///////////////////////////////////////////////////////////////////////////////
// operator new 
// EASTL requires the following new operators to be defined.
//
void* operator new[](size_t size, const char*, int, unsigned, const char*, int)
{
	return new char[size];
}

void* operator new[](size_t size, size_t, size_t, const char*, int, unsigned, const char*, int)
{
	return new char[size];
}



///////////////////////////////////////////////////////////////////////////////
// main
//
int EAMain(int argc, char** argv)
{
	using namespace EA::Thread;
	using namespace EA::UnitTest;
	using namespace EA::StdC;

	int  nErrorCount = 0;
	bool bDebugMode  = false;

	EA::EAMain::PlatformStartup();

	// Process possible command line parameters
	for(int i(0); i < argc; i++)
	{
		// Look for -?
		if(Stricmp(argv[i], "-?") == 0)
		{
			printf("Command line parameters:\n");
			printf("    -?            Get Help.\n");
			printf("    -t <seconds>  Run the tests for <seconds> seconds each.\n");
			printf("    -c <count>    Specifies test thread count.\n");
			printf("    -d            Debug mode. Causes app to wait for a debugger to connect.\n");
			continue;
		}

		// Run the tests for <seconds> seconds each.
		if((Stricmp(argv[i], "-t") == 0) && (i < (argc - 1)))
		{
			gTestLengthSeconds = (unsigned int) atoi(argv[i+1]);
			if(gTestLengthSeconds < 3)
				gTestLengthSeconds = 3;
			continue;
		}

		// Specifies test thread count. e.g. -c 10
		if((Stricmp(argv[i], "-c") == 0) && (i < (argc - 1)))
		{
			gTestThreadCount = (unsigned int) atoi(argv[i+1]);
			if(gTestThreadCount < 1)
				gTestThreadCount = 1;
			if(gTestThreadCount > 100)
				gTestThreadCount = 100;
			continue;
		}

		// Debug mode. Causes app to wait for a debugger to connect.
		if(Stricmp(argv[i], "-d") == 0)
		{
			bDebugMode = true;
			continue;
		}
	}


	// Set EAThread to route its errors to our own error reporting function.
	EA::Thread::SetAssertionFailureFunction(EAThreadFailure, NULL);

	ReportVerbosity(1, "Test time seconds: %u\n", gTestLengthSeconds);
	ReportVerbosity(1, "Thread count: %u\n", gTestThreadCount);

	// Print ThreadId for this primary thread.
	const ThreadId threadId = GetThreadId();
	ReportVerbosity(1, "Primary thread ThreadId: %08x\n", (int)(intptr_t)threadId);

	// Print SysThreadId for this primary thread.
	const SysThreadId sysThreadId = GetSysThreadId(threadId);
	ReportVerbosity(1, "Primary thread SysThreadId: %08d\n", (int)(intptr_t)sysThreadId);

	// Print thread priority for this primary thread.
	const int nPriority = EA::Thread::GetThreadPriority();
	ReportVerbosity(1, "Primary thread priority: %d\n", nPriority);

	const int nProcessorCount = EA::Thread::GetProcessorCount();
	ReportVerbosity(1, "Currently active virtual processor count: %d\n", nProcessorCount);

	if(bDebugMode)
	{
		Report("Debug mode activated. Waiting for debugger to attach.\n");
		while(bDebugMode)
			ThreadSleepRandom(500, 500, true);
		Report("Continuing.\n");
	}

	// Add the tests
	TestApplication testSuite("EAThread Interprocess Unit Tests", argc, argv);

	testSuite.AddTest("RWMutex", TestThreadRWMutex);

	nErrorCount += testSuite.Run();

	EA::EAMain::PlatformShutdown(nErrorCount);

	return nErrorCount;
}









