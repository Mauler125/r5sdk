#pragma once

#include <cstdint>
#include <atomic>
#include "Job.h"

namespace Jobs
{
	// Forward declared because c++
	class JobManager;

	class JobWorker
	{
	public:
		JobWorker();
		JobWorker(JobManager* Manager, uint32_t WorkerId = 0);
		~JobWorker() = default;

		// The main thread look for this worker.
		void Run();
		// Halt this threads execution.
		void Stop();

		// Whether or not the worker is executing a job or is looking for one.
		bool IsExecuting();

	private:
		// Try and execute a task if we can
		void TryRunTask();
		// Fetch a job to run on this worker
		Job* GetJob();

		JobManager* _Manager;
		uint32_t _WorkerId;
		std::atomic_bool _Running;
		std::atomic_bool _Executing;
	};
}