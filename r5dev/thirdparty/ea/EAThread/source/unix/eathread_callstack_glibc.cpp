///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#include <eathread/eathread_callstack.h>
#include <eathread/eathread_callstack_context.h>
#include <string.h>
#include <pthread.h>

#include <eathread/eathread_storage.h>

namespace EA
{
namespace Thread
{


// To do: Remove the usage of sStackBase for the platforms that it's not needed,
// as can be seen from the logic below. For example Mac OSX probably doesn't need it.
static EA::Thread::ThreadLocalStorage sStackBase;

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
	#if defined(EA_PLATFORM_UNIX)
		void* pBase;
		if(GetPthreadStackInfo(&pBase, NULL))
			return pBase;
	#endif

	// Else we require the user to have set this previously, usually via a call 
	// to SetStackBase() in the start function of this currently executing 
	// thread (or main for the main thread).
	return sStackBase.GetValue();
}


///////////////////////////////////////////////////////////////////////////////
// GetStackLimit
//
EATHREADLIB_API void* GetStackLimit()
{
	#if defined(EA_PLATFORM_UNIX)
		void* pLimit;
		if(GetPthreadStackInfo(NULL, &pLimit))
			return pLimit;
	#endif

	// If this fails then we might have an issue where you are using GCC but not 
	// using the GCC standard library glibc. Or maybe glibc doesn't support 
	// __builtin_frame_address on this platform. Or maybe you aren't using GCC but
	// rather a compiler that masquerades as GCC (common situation).
	void* pStack = __builtin_frame_address(0);
	return (void*)((uintptr_t)pStack & ~4095); // Round down to nearest page, as the stack grows downward.

}


} // namespace Thread
} // namespace EA



