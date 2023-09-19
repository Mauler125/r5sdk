#include "stdafx.h"
#include "JobManager.h"

namespace Jobs
{
	JobManager::JobManager(const uint32_t Workers, uint32_t DefaultWorkerId)
		: _IsActive(true), _OwnerThreadId(std::this_thread::get_id())
	{
		for (uint32_t i = 0; i < Workers; i++)
		{
			_Workers.EmplaceBack(new JobWorker(this, DefaultWorkerId));
			_Threads.Emplace([this, i]()
			{
				_Workers[i]->Run();
			}).Start();
		}
	}

	JobManager::~JobManager()
	{
		this->HaltAllJobs();
	}

	Job* JobManager::ScheduleJob(std::function<void()> Task, std::function<void()> OnFinished)
	{
		if (!this->_IsActive)
			return nullptr;

		auto NewJob = new Job(Task, OnFinished, true);
		_JobPool.Enqueue(NewJob);

		if (!this->_IsActive)
		{
			delete NewJob;
			return nullptr;
		}

		return NewJob;
	}

	void JobManager::ScheduleJobNoWait(std::function<void()> Task, std::function<void()> OnFinished)
	{
		if (!this->_IsActive)
			return;

		auto NewJob = new Job(Task, OnFinished, false);
		_JobPool.Enqueue(NewJob);

		if (!this->_IsActive)
		{
			delete NewJob;
		}
	}

	void JobManager::WaitForSingleJob(Job* Job)
	{
		while (!Job->IsFinished())
			std::this_thread::yield();

		delete Job;
	}

	void JobManager::WaitForAllJobs(Job** Jobs, const uint32_t Count)
	{
		bool AllFinished = false;
		while (!AllFinished)
		{
			AllFinished = true;
			for (uint32_t i = 0; i < Count; i++)
			{
				if (Jobs[i] != nullptr && !Jobs[i]->IsFinished())
				{
					AllFinished = false;
					break;
				}
			}

			if (AllFinished)
				break;

			std::this_thread::yield();
		}

		for (uint32_t i = 0; i < Count; i++)
			delete Jobs[i];
	}

	void JobManager::WaitForAllJobs(List<Job*>& Jobs)
	{
		this->WaitForAllJobs(&Jobs[0], Jobs.Count());
	}

	void JobManager::WaitAll(std::function<void(uint32_t)> ProgressCallback, std::function<bool()> CancelCallback)
	{
		// Must be called by owner thread
		if (std::this_thread::get_id() != _OwnerThreadId)
			throw std::exception("Must be called by thread that owns JobManager!");

		// We can store this here, just for dependency tracking...
		uint32_t StorageLeft = this->_JobPool.Count();
		std::atomic<uint32_t> LastProgress = 0;

		// Flags brought to stack for easy read information...
		auto isPoolEmpty = this->_JobPool.IsEmpty();
		auto isWorkersIdle = this->AreWorkersIdle();

		while (!isPoolEmpty || !isWorkersIdle)
		{
			std::this_thread::yield();

			isPoolEmpty = this->_JobPool.IsEmpty();
			isWorkersIdle = this->AreWorkersIdle();

			if (ProgressCallback != nullptr)
			{
				auto TempStorage = this->_JobPool.Count();

				if (TempStorage > StorageLeft)
				{
					// We got more jobs in the process of processing other jobs
					// so the progress is shifted
					StorageLeft = TempStorage;
					
					if (LastProgress != 0)
					{
						LastProgress = 0;
						ProgressCallback(0);
					}
				}
				else
				{
					// We can convert our progress range here, then abs the value
					int32_t ProgressRange = (TempStorage - StorageLeft);
					uint32_t Progress = std::abs((int32_t)(((float)ProgressRange / (float)StorageLeft) * 100.0f));

					if (LastProgress != Progress)
					{
						LastProgress = Progress;
						ProgressCallback(Progress);
					}
				}
			}

			// Break out of the loop if the setup was canceled
			if (!this->_IsActive)
				break;

			// Check if a cancel callback was provided, and if cancel is confirmed
			if (CancelCallback != nullptr && CancelCallback())
			{
				this->Cancel();
				break;
			}
		}
	}

	void JobManager::HaltAllJobs()
	{
		// Must be called by owner thread
		if (std::this_thread::get_id() != _OwnerThreadId)
			throw std::exception("Must be called by thread that owns JobManager!");

		// Prevents the scheduler from scheduling more jobs
		this->_IsActive = false;

		// Prevent execution of threads...
		for (auto& Worker : _Workers)
			Worker->Stop();

		// Wait for the last jobs to comlete, then destroy the workers
		for (auto& Thread : _Threads)
			Thread.Join();
		for (auto& Worker : _Workers)
			delete Worker;

		// Iterate, and delete all jobs being non-waited, and mark all jobs awaited as finished.
		Job* Item = nullptr;
		while (_JobPool.Dequeue(Item))
		{
			if (!Item->IsAwaiter())
			{
				// It's not being awaited on so we must free it here...
				delete Item;
			}
			else
			{
				// Forceable mark as finished, so the awaiter elsewhere can stop and clean up.
				Item->IsFinished();
			}
		}

		// Make sure the buffers are clear for subsequent calls
		_Workers.Clear();
		_Threads.Clear();
	}

	void JobManager::Cancel()
	{
		this->_IsActive = false;

		for (auto& Worker : _Workers)
			Worker->Stop();
	}

	bool JobManager::AreWorkersIdle()
	{
		for (auto& Worker : _Workers)
		{
			if (Worker->IsExecuting())
				return false;
		}

		return true;
	}

	Job* JobWorker::GetJob()
	{
		Job* Item = nullptr;
		auto Result = _Manager->_JobPool.Dequeue(Item);
		return (Result) ? Item : nullptr;
	}
}
