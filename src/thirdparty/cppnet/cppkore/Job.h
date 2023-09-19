#pragma once

#include <cstdint>
#include <atomic>
#include <new>
#include <functional>
#include <utility>

namespace Jobs
{
	// Represents a single task to complete on any thread, with optionally a task to complete when finished.
	class Job
	{
	public:
		Job();
		Job(std::function<void()> Task, std::function<void()> OnFinished, bool IsAwaiter);
		~Job() = default;

		// Executes the job, sets the state to finished, and calls the finalizer.
		void Execute();
		// Forcefully sets the job state to finished.
		void SetIsFinished();
		// Returns whether or not the job is completed.
		bool IsFinished();
		// Returns whether or not the job is being used as an awaitable object.
		bool IsAwaiter();

	private:
		// Internal cached states for the job
		std::function<void()> _Task;
		std::function<void()> _Finished;
		std::atomic_bool _IsFinished;
		std::atomic_bool _IsAwaiter;
	};
}