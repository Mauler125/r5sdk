///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include <eathread/eathread_callstack.h>
#include <eathread/eathread_callstack_context.h>
#include <eathread/eathread_storage.h>
#include <string.h>


EA_DISABLE_VC_WARNING(4172) // returning address of local variable or temporary


namespace EA
{
namespace Thread
{


#if EA_THREADS_AVAILABLE
	static EA::Thread::ThreadLocalStorage sStackBase;
#else
	static void* sStackBase;
#endif

///////////////////////////////////////////////////////////////////////////////
// SetStackBase
//
EATHREADLIB_API void SetStackBase(void* pStackBase)
{
	if(pStackBase)
	{
		#if EA_THREADS_AVAILABLE
			sStackBase.SetValue(pStackBase);
		#else
			sStackBase = pStackBase;
		#endif
	}
	else
	{
		pStackBase = GetStackBase();
		SetStackBase(pStackBase);
	}
}


///////////////////////////////////////////////////////////////////////////////
// GetStackBase
//
EATHREADLIB_API void* GetStackBase()
{
	#if EA_THREADS_AVAILABLE
		void* pStackBase = sStackBase.GetValue();
	#else
		void* pStackBase = sStackBase;
	#endif

	if(!pStackBase)
		pStackBase = (void*)(((uintptr_t)GetStackLimit() + 4095) & ~4095); // Align up to nearest page, as the stack grows downward.

	return pStackBase;
}


///////////////////////////////////////////////////////////////////////////////
// GetStackLimit
//
EATHREADLIB_API void* GetStackLimit()
{
	void* pStack = NULL;

	pStack = &pStack;

	return (void*)((uintptr_t)pStack & ~4095); // Round down to nearest page, as the stack grows downward.
}

} // namespace Thread
} // namespace EA

EA_RESTORE_VC_WARNING()

