///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include <EABase/eabase.h>
#include <eathread/eathread.h>
#include <eathread/eathread_atomic.h>
#include <eathread/eathread_callstack.h>
#include <eathread/eathread_callstack_context.h>
#include <eathread/eathread_storage.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <unwind.h>

namespace EA
{
namespace Thread
{

EA::Thread::ThreadLocalStorage sStackBase;

///////////////////////////////////////////////////////////////////////////////
// SetStackBase
//
EATHREADLIB_API void SetStackBase(void* pStackBase)
{
	if(pStackBase)
		sStackBase.SetValue(pStackBase);
	else
	{
		pStackBase = __builtin_frame_address(0);

		if(pStackBase)
			SetStackBase(pStackBase);
		// Else failure; do nothing.
	}
}


///////////////////////////////////////////////////////////////////////////////
// GetStackBase
//
EATHREADLIB_API void* GetStackBase()
{
	void* pBase;

	if(GetPthreadStackInfo(&pBase, NULL))
		return pBase;

	// Else we require the user to have set this previously, usually via a call 
	// to SetStackBase() in the start function of this currently executing 
	// thread (or main for the main thread).
	pBase = sStackBase.GetValue();

	if(pBase == NULL)
		pBase = (void*)(((uintptr_t)&pBase + 4095) & ~4095); // Make a guess, round up to next 4096.

	return pBase;
}


///////////////////////////////////////////////////////////////////////////////
// GetStackLimit
//
EATHREADLIB_API void* GetStackLimit()
{
	void* pLimit;

	if(GetPthreadStackInfo(NULL, &pLimit))
		return pLimit;

	pLimit = __builtin_frame_address(0);

	return (void*)((uintptr_t)pLimit & ~4095); // Round down to nearest page, as the stack grows downward.
}



} // namespace Thread
} // namespace EA







