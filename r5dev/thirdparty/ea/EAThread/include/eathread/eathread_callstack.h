///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif

#ifndef EATHREAD_EATHREAD_CALLSTACK_H
#define EATHREAD_EATHREAD_CALLSTACK_H

#include <EABase/eabase.h>
#include <eathread/eathread.h>
#include <stddef.h>

namespace EA
{
	namespace Thread
	{
		#if defined(EA_PLATFORM_MICROSOFT)
			/// This function is the same as EA::Thread::GetSysThreadId(ThreadId id).
			/// This function converts from one type of Microsoft thread identifier to another.
			/// threadId is the same as EA::Thread::ThreadId and is a Microsoft thread HANDLE.
			/// The return value is a Microsoft DWORD thread id which is the same as EA::Thread::SysThreadId.
			/// Upon failure, the return value will be zero.
			EATHREADLIB_API uint32_t GetThreadIdFromThreadHandle(intptr_t threadId);
		#endif

		/// EAGetInstructionPointer / GetInstructionPointer
		///
		/// Returns the current instruction pointer (a.k.a. program counter).
		/// This function is implemented as a macro, it acts as if its declaration
		/// were like so:
		///     void EAGetInstructionPointer(void*& p);
		///
		/// For portability, this function should only be used as a standalone
		/// statement on its own line.
		///
		/// These functions return NULL/0 on error or if getting the value is not possible
		///
		/// Example usage:
		///    void* pInstruction;
		///    EAGetInstructionPointer(pInstruction);
		///
		#if defined(EA_COMPILER_MSVC)

			EATHREADLIB_API EA_NO_INLINE void GetInstructionPointer(void*& pInstruction);

			#define EAGetInstructionPointer(p) EA::Thread::GetInstructionPointer(p)

		#elif defined(EA_COMPILER_GNUC) || defined(EA_COMPILER_CLANG)

			EATHREADLIB_API EA_NO_INLINE void GetInstructionPointer(void*& pInstruction);

			#define EAGetInstructionPointer(p) EA::Thread::GetInstructionPointer(p)

		#elif defined(EA_COMPILER_ARM) || defined(EA_COMPILER_RVCT)

			#define EAGetInstructionPointer(p)		\
				{											\
					p = static_cast<void*>(__current_pc()); \
				}

			EA_FORCE_INLINE void GetInstructionPointer(void*& p)
			{
				EAGetInstructionPointer(p);
			}

		#else

			#error "EAGetInstructionPointer() Not Supported On The Given Platform/Compiler!!!"

		#endif


		/// EASetStackBase / SetStackBase / GetStackBase / GetStackLimit
		///
		/// EASetStackBase as a macro and acts as if its declaration were like so:
		///     void EASetStackBase();
		///
		/// EASetStackBase sets the current stack pointer as the bottom (beginning)
		/// of the stack. Depending on the platform, the "bottom" may be up or down
		/// depending on whether the stack grows upward or downward (usually it grows
		/// downward and so "bottom" actually refers to an address that is above child
		/// stack frames in memory.
		/// This function is intended to be called on application startup as early as
		/// possible, and in each created thread, as early as possible. Its purpose
		/// is to record the beginning stack pointer because the platform doesn't provide
		/// APIs to tell what it is, and we need to know it (e.g. so we don't overrun
		/// it during stack unwinds).
		///
		/// For portability, EASetStackBase should be used only as a standalone
		/// statement on its own line, as it may include statements that can't work otherwise.
		///
		/// Example usage:
		///    int main(int argc, char** argv) {
		///       EASetStackBase();
		///       . . .
		///    }
		///
		/// SetStackBase is a function which lets you explicitly set a stack bottom instead
		/// of doing it automatically with EASetStackBase. If you pass NULL for pStackBase
		/// then the function uses its stack location during its execution, which will be
		/// a little less optimal than calling EASetStackBase.
		///
		/// GetStackBase returns the stack bottom set by EASetStackBase or SetStackBase.
		/// It returns NULL if no stack bottom was set or could be set.
		///
		/// GetStackLimit returns the current stack "top", which will be lower than the stack
		/// bottom in memory if the platform grows its stack downward.

		EATHREADLIB_API void  SetStackBase(void* pStackBase);
		inline          void  SetStackBase(uintptr_t pStackBase){ SetStackBase((void*)pStackBase); }
		EATHREADLIB_API void* GetStackBase();
		EATHREADLIB_API void* GetStackLimit();


		#if defined(EA_COMPILER_MSVC) && defined(EA_PROCESSOR_X86)
			#define EASetStackBase()               \
			{                                      \
				void* esp;                         \
				__asm { mov esp, ESP }             \
				::EA::Thread::SetStackBase(esp);   \
			}

		#elif defined(EA_COMPILER_MSVC) && (defined(EA_PROCESSOR_X86_64) || defined(EA_PROCESSOR_ARM))
			// This implementation uses SetStackBase(NULL), which internally retrieves the stack pointer.
			#define EASetStackBase()                     \
			{                                            \
				::EA::Thread::SetStackBase((void*)NULL); \
			}                                            \

		#elif defined(EA_COMPILER_ARM)          // ARM compiler

			#define EASetStackBase()  \
				::EA::Thread::SetStackBase((void*)__current_sp())

		#elif defined(EA_COMPILER_GNUC) || defined(EA_COMPILER_CLANG) // This covers EA_PLATFORM_UNIX, EA_PLATFORM_OSX

			#define EASetStackBase()  \
				::EA::Thread::SetStackBase((void*)__builtin_frame_address(0));

		#else
			// This implementation uses SetStackBase(NULL), which internally retrieves the stack pointer.
			#define EASetStackBase()                     \
			{                                            \
				::EA::Thread::SetStackBase((void*)NULL); \
			}                                            \

		#endif

		#if defined(EA_PLATFORM_UNIX) || defined(EA_PLATFORM_APPLE) || defined(EA_PLATFORM_SONY)
			// GetPthreadStackInfo
			//
			// With some implementations of pthread, the stack base is returned by pthread as NULL if it's the main thread,
			// or possibly if it's a thread you created but didn't call pthread_attr_setstack manually to provide your
			// own stack. It's impossible for us to tell here whether will be such a NULL return value, so we just do what
			// we can and the user nees to beware that a NULL return value means that the system doesn't provide the
			// given information for the current thread. This function returns false and sets pBase and pLimit to NULL in
			// the case that the thread base and limit weren't returned by the system or were returned as NULL.

			bool GetPthreadStackInfo(void** pBase, void** pLimit);
		#endif

	} // namespace Thread

} // namespace EA


#endif // Header include guard.
