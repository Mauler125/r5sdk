#include "stdafx.h"
#include "Thread.h"

namespace Threading
{
	Thread::Thread()
		: _Handle(NULL), _Param(nullptr), _Start(nullptr), _ParamStart(nullptr), _DisposeWait(false)
	{
	}

	Thread::Thread(ThreadStart Start)
		: _Handle(NULL), _Param(nullptr), _Start(Start), _ParamStart(nullptr), _DisposeWait(false)
	{
	}

	Thread::Thread(ParameterizedThreadStart Start)
		: _Handle(NULL), _Param(nullptr), _ParamStart(Start), _Start(nullptr), _DisposeWait(false)
	{
	}

	Thread::~Thread()
	{
		while (_DisposeWait) Sleep(1);

		if (this->_Handle)
			CloseHandle(this->_Handle);
		this->_Handle = NULL;
	}

	void Thread::Start()
	{
		if (this->_Handle != NULL)
			return;	// Already started

		this->_DisposeWait = true;
		this->_Handle = CreateThread(NULL, 0, Thread::FunctionTrampoline, this, 0, NULL);
	}

	void Thread::Start(void* Object)
	{
		if (this->_Handle != NULL)
			return;	// Already started

		this->_DisposeWait = true;
		this->_Param = Object;
		this->_Handle = CreateThread(NULL, 0, Thread::FunctionTrampoline, this, 0, NULL);
	}

	void Thread::Suspend()
	{
		if (this->_Handle == NULL)
			return;

		SuspendThread(this->_Handle);
	}

	void Thread::Resume()
	{
		if (this->_Handle == NULL)
			return;

		ResumeThread(this->_Handle);
	}

	void Thread::Abort()
	{
		if (this->_Handle == NULL)
			return;

		TerminateThread(this->_Handle, 0);
	}

	void Thread::Join()
	{
		if (this->_Handle == NULL)
			return;

		WaitForSingleObject(this->_Handle, INFINITE);
	}
}