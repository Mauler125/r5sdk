///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include <eathread/eathread_callstack.h>
#include <eathread/eathread_callstack_context.h>
#include <stdio.h>
#include <string.h>
#include <eathread/eathread_storage.h>

#pragma warning(push, 0)
#include <Windows.h>
#pragma warning(pop)

namespace EA
{
namespace Thread
{

///////////////////////////////////////////////////////////////////////////////
// GetThreadIdFromThreadHandle
//
// This implementation is the same as the one in EAThread.
// Converts a thread HANDLE (threadId) to a thread id DWORD (sysThreadId).
// Recall that Windows has two independent thread identifier types.
//
EATHREADLIB_API uint32_t GetThreadIdFromThreadHandle(intptr_t threadId)
{
	// Win64 has this function natively, unlike earlier versions of 32 bit Windows.
	return (uint32_t)::GetThreadId((HANDLE)threadId);
}


///////////////////////////////////////////////////////////////////////////////
// SetStackBase
//
EATHREADLIB_API void SetStackBase(void* /*pStackBase*/)
{
	// Nothing to do, as GetStackBase always works on its own.
}

///////////////////////////////////////////////////////////////////////////////
// GetStackBase
//
EATHREADLIB_API void* GetStackBase()
{
	NT_TIB64* pTIB = (NT_TIB64*)NtCurrentTeb(); // NtCurrentTeb is defined in <WinNT.h> as an inline call to __readgsqword
	return (void*)pTIB->StackBase;
}


///////////////////////////////////////////////////////////////////////////////
// GetStackLimit
//
EATHREADLIB_API void* GetStackLimit()
{
	NT_TIB64* pTIB = (NT_TIB64*)NtCurrentTeb(); // NtCurrentTeb is defined in <WinNT.h> as an inline call to __readgsqword
	return (void*)pTIB->StackLimit;

	// The following is an alternative implementation that returns the extent
	// of the current stack usage as opposed to the stack limit as seen by the OS.
	// This value will be a higher address than Tib.StackLimit (recall that the
	// stack grows downward). It's debatable which of these two approaches is
	// better, as one returns the thread's -usable- stack space while the
	// other returns how much the thread is -currently- using. The determination
	// of the usable stack space is complicated by the fact that Microsoft
	// platforms auto-extend the stack if the process pushes beyond the current limit.
	// In the end the Tib.StackLimit solution is actually the most portable across
	// Microsoft OSs and compilers for those OSs (Microsoft or not).

	// Alternative implementation:
	// We return our stack pointer, which is a good approximation of the stack limit of the caller.
	// void* rsp = GetRSP();
	// return rsp;
}


} // namespace Thread
} // namespace EA
