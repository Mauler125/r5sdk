#pragma once

#include <memory>
#include <Windows.h>
#include <functional>
#include "ThreadStart.h"

namespace Threading
{
	class Thread
	{
	public:
		Thread();
		Thread(ThreadStart Start);
		Thread(ParameterizedThreadStart Start);

		~Thread();

		// Starts the threads execution
		void Start();
		// Starts the threads execution with the given argument
		void Start(void* Object);
		// Suspend the current threads execution if it's running
		void Suspend();
		// Resumt the current threads if it's suspended
		void Resume();
		// Attempt to cancel the threads execution
		void Abort();
		// Wait for the thread to finish execution
		void Join();

	private:
		// Cached thread flags
		HANDLE _Handle;
		std::function<void(void)> _Start;
		std::function<void(void*)> _ParamStart;
		void* _Param;

		// When we can dispose
		volatile bool _DisposeWait;

		// Internal trampoline designed to automatically wrap the function
		inline static DWORD WINAPI FunctionTrampoline(LPVOID lpParam);
	};

	inline DWORD WINAPI Thread::FunctionTrampoline(LPVOID lpParam)
	{
		auto thisState = (Thread*)lpParam;

		auto ParamStart = thisState->_ParamStart;
		auto Param = thisState->_Param;
		auto Start = thisState->_Start;

		thisState->_DisposeWait = false;

		if (ParamStart != nullptr)
			ParamStart(Param);
		else
			Start();

		return 0;
	}
}