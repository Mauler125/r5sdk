///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include <EATest/EATest.h>
#include <eathread/eathread.h>
#include <eathread/eathread_atomic.h>
#include <eathread/eathread_thread.h>
#include <EAStdC/EAString.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <EAMain/EAEntryPointMain.inl>


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

const int NUM_THREADS = 5;

///////////////////////////////////////////////////////////////////////////////
// Structure where a single instance is passed to every active thread.
//
struct GlobalData
{
	EA::Thread::AtomicInt32 i;    
	GlobalData()
	: i(0)    
	{}
};

typedef void (*DLL_ENTRY)(GlobalData*);

///////////////////////////////////////////////////////////////////////////////
// Thread Entry 
//
intptr_t ThreadFunction(void* pData)
{
	GlobalData* const pGlobalData = static_cast<GlobalData*>(pData);
	while(!pGlobalData->i)
	{
		EA::Thread::ThreadSleep(50);
	}

	return 0;
}


///////////////////////////////////////////////////////////////////////////////
// The function export below MUST be here as it forces lib files to be 
// generated for the DLL's being built.
//
extern "C"
EA_EXPORT void ForceLibFilesToBeGenerated(GlobalData* pGlobal)
{
	using namespace EA::Thread;
	using namespace EA::UnitTest;

	// Start two threads in this DLL instance
	Thread thread;
	for(size_t i = 0; i < NUM_THREADS; i++)
		thread.Begin(ThreadFunction, pGlobal);
			   
	// Get the number of threads in the system 
	ThreadEnumData enumData[32];
	size_t count = EnumerateThreads(enumData, EAArrayCount(enumData));
	//Report("Number of DLL threads detected:  %d\n", count);

	for(size_t i = 0; i < count; i++)
		enumData[i].Release();    
}

///////////////////////////////////////////////////////////////////////////////
// Main
//
int EAMain(int argc, char** argv)
{
	using namespace EA::Thread;
	using namespace EA::UnitTest;

	int  nErrorCount = 0;    
	GlobalData data;

// TODO:  DLL usage must be made portable through support at various levels of our technology stack.
#if defined(EA_PLATFORM_MICROSOFT) && defined(EA_PLATFORM_WIN32)
	HINSTANCE__* mod1 = LoadLibrary("EAThreadTestDllSafetyMod1.dll");
	HINSTANCE__* mod2 = LoadLibrary("EAThreadTestDllSafetyMod2.dll");
	DLL_ENTRY dllmain1 = (DLL_ENTRY)GetProcAddress(mod1, "ForceLibFilesToBeGenerated");
	DLL_ENTRY dllmain2 = (DLL_ENTRY)GetProcAddress(mod2, "ForceLibFilesToBeGenerated");

	if(dllmain1 != NULL) 
		dllmain1(&data);

	if(dllmain2 != NULL) 
		dllmain2(&data);
#endif

	EA::EAMain::PlatformStartup();
	{
		// Start n threads in this DLL instance
		Thread thread;
		for(size_t i = 0; i < NUM_THREADS; i++)
			thread.Begin(ThreadFunction, &data);
			   
		// Get the number of threads in the system and validate.
		ThreadEnumData enumData[32];
		size_t count = EnumerateThreads(enumData, EAArrayCount(enumData));
		Report("Number of threads detected:  %d/%d. \n", count, NUM_THREADS*3);
		EATEST_VERIFY_MSG(count >= NUM_THREADS, "Thread tracking data isn't DLL safe.  We are missing data generated in other DLL's.");

		for(size_t i = 0; i < count; i++)
			enumData[i].Release();
		
		// Release the threads
		data.i++;
	}
	EA::EAMain::PlatformShutdown(nErrorCount);

#if defined(EA_PLATFORM_MICROSOFT) && defined(EA_PLATFORM_WIN32)
	FreeLibrary(mod1); 
	FreeLibrary(mod2);
#endif

	return nErrorCount;
}






