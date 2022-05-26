#pragma once

#include <cstdint>
#include <memory>
#include <functional>
#include "ListBase.h"
#include "Thread.h"

namespace Threading
{
	// Handles running a task in parallel
	class ParallelTask
	{
	public:

		// Runs a task in parallel given the thread count
		ParallelTask(const std::function<void(void)> Task, uint32_t DegreeOfParallelism = 0)
		{
			// If value is 0, adjust to core count
			if (DegreeOfParallelism == 0)
			{
				SYSTEM_INFO SysInfo{};
				GetSystemInfo(&SysInfo);

				// We must always have one thread
				DegreeOfParallelism = max((uint32_t)1, (uint32_t)SysInfo.dwNumberOfProcessors);
			}

			// Worker pool
			List<Thread> Workers;

			// Loop and add the jobs
			for (uint32_t i = 0; i < DegreeOfParallelism; i++)
				Workers.Emplace(Task).Start();

			// Wait for all workers to end
			for (auto& Worker : Workers)
			{
				try
				{
					Worker.Join();
				}
				catch (...)
				{
				}
			}
		}
	};
}