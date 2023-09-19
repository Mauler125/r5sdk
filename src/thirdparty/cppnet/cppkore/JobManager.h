#pragma once

#include <cstdint>
#include <array>
#include <atomic>
#include <thread>
#include "ListBase.h"
#include "Job.h"
#include "JobWorker.h"
#include "Thread.h"
#include "AtomicListBase.h"

namespace Jobs
{
	// Remove built-in windows function
#undef GetJob

	// The job manager is the base class for all async operations.
	// It handles scheduling jobs, waiting on jobs, and allocating jobs.
	class JobManager
	{
	public:
		// Initialize a new job manager, with the specified amount of 'workers' (threads)
		JobManager(const uint32_t Workers, uint32_t DefaultWorkerId = 0);
		~JobManager();

		// Schedules a job for execution, this returns a Job task for use with waiting for it's execution to finish.
		// you must dispose of the resource after you're done with it
		[[nodiscard]] Job* ScheduleJob(std::function<void()> Task, std::function<void()> OnFinished = nullptr);

		// Schedules a job for execution, this automatically cleans up the job after it's done with.
		void ScheduleJobNoWait(std::function<void()> Task, std::function<void()> OnFinished = nullptr);

		// Waits for a single job to continue it's execution, yielding the thread until it's completed.
		// After, the job is cleanly disposed.
		void WaitForSingleJob(Job* Job);

		// Waits for multiple jobs to continue their execution, yielding the thread until all of them have completed.
		// After, all jobs are disposed.
		void WaitForAllJobs(Job** Jobs, const uint32_t Count);
		// Waits for multiple jobs to continue their execution, yielding the thread until all of them have completed.
		// After, all jobs are disposed.
		void WaitForAllJobs(List<Job*>& Jobs);

		// Waits for the entire queue to finish execution
		// When this ends, all scheduled jobs are complete.
		// (Must be called by owning thread)
		void WaitAll(std::function<void(uint32_t)> ProgressCallback = nullptr, std::function<bool()> CancelCallback = nullptr);

		// Stops all threads, waits for the final execution, and frees up all resources required.
		// (Must be called by owning thread)
		void HaltAllJobs();

		// Cancels all current jobs, and addition of new jobs.
		void Cancel();

		// Allow the job worker to access private members of the manager
		friend class JobWorker;
	private:
		// Workers will use our manager to get jobs
		List<JobWorker*> _Workers;
		List<Threading::Thread> _Threads;

		// Internal routine to check if all workers are idle and not executing.
		bool AreWorkersIdle();

		// Fixed size atomoc job queue
		AtomicListBase<Job*> _JobPool;
		std::thread::id _OwnerThreadId;
		std::atomic_bool _IsActive;
	};
}