#pragma once

#include "Thread.h"
#include "ThreadStart.h"

namespace Threading
{
	class Task
	{
	public:
		// Starts the specified action on a new thread
		inline static void Run(ThreadStart Action)
		{
			Thread(Action).Start();
		}
		// Starts the specified action on a new thread with the given object
		inline static void Run(ParameterizedThreadStart Action, void* Object)
		{
			Thread(Action).Start(Object);
		}
		// Starts the specified action on a new thread and waits for it to exit
		inline static void RunAwait(ThreadStart Action)
		{
			auto Runner = Thread(Action);

			Runner.Start();
			Runner.Join();
		}
		// Starts the specified action on a new thread with the given object and waits for it to exit
		inline static void RunAwait(ParameterizedThreadStart Action, void* Object)
		{
			auto Runner = Thread(Action);

			Runner.Start(Object);
			Runner.Join();
		}
	};
}