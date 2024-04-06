///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include <EABase/eabase.h>
#include <eathread/eathread_callstack.h>
#include <eathread/eathread_callstack_context.h>
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
//
EATHREADLIB_API uint32_t GetThreadIdFromThreadHandle(intptr_t threadId)
{
	struct THREAD_BASIC_INFORMATION_WIN32
	{
		BOOL  ExitStatus;
		PVOID TebBaseAddress;
		DWORD UniqueProcessId;
		DWORD UniqueThreadId;
		DWORD AffinityMask;
		DWORD Priority;
		DWORD BasePriority;
	};

	static HMODULE hKernel32 = NULL;
	if (!hKernel32)
		hKernel32 = LoadLibraryA("kernel32.dll");

	if (hKernel32)
	{
		typedef DWORD (WINAPI *GetThreadIdFunc)(HANDLE);

		static GetThreadIdFunc pGetThreadIdFunc = NULL;
		if (!pGetThreadIdFunc)
			pGetThreadIdFunc = (GetThreadIdFunc)(uintptr_t)GetProcAddress(hKernel32, "GetThreadId");

		if (pGetThreadIdFunc)
			return pGetThreadIdFunc((HANDLE)threadId);
	}


	static HMODULE hNTDLL = NULL;
	if (!hNTDLL)
		hNTDLL = LoadLibraryA("ntdll.dll");

	if (hNTDLL)
	{
		typedef LONG (WINAPI *NtQueryInformationThreadFunc)(HANDLE, int, PVOID, ULONG, PULONG);

		static NtQueryInformationThreadFunc pNtQueryInformationThread = NULL;
		if (!pNtQueryInformationThread)
			pNtQueryInformationThread = (NtQueryInformationThreadFunc)(uintptr_t)GetProcAddress(hNTDLL, "NtQueryInformationThread");

		if (pNtQueryInformationThread)
		{
			THREAD_BASIC_INFORMATION_WIN32 tbi;

			if(pNtQueryInformationThread((HANDLE)threadId, 0, &tbi, sizeof(tbi), NULL) == 0)
				return tbi.UniqueThreadId;
		}
	}

	return 0;
}

namespace Internal
{

struct TIBStackInfo
{
	uintptr_t StackBase;
	uintptr_t StackLimit;
};

static TIBStackInfo GetStackInfo()
{
	NT_TIB*     pTib;

	/**
	 * Offset 0x18 from the FS segment register gives a pointer to
	 * the thread information block for the current thread
	 * https://en.wikipedia.org/wiki/Win32_Thread_Information_Block
	 */
	__asm {
		mov eax, fs:[18h]
		mov pTib, eax
	}

	return { ((uintptr_t)pTib->StackBase), ((uintptr_t)pTib->StackLimit) };
}

} // namespace Internal

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
	Internal::TIBStackInfo info = Internal::GetStackInfo();

	return ((void*)info.StackBase);
}


///////////////////////////////////////////////////////////////////////////////
// GetStackLimit
//
EATHREADLIB_API void* GetStackLimit()
{
	Internal::TIBStackInfo info = Internal::GetStackInfo();

	return ((void*)info.StackLimit);

	// Alternative which returns a slightly different answer:
	// We return our stack pointer, which is a good approximation of the stack limit of the caller.
	// void* pStack = NULL;
	// __asm { mov pStack, ESP};
	// return pStack;
}


} // namespace Thread
} // namespace EA
