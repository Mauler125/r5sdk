#include "stdafx.h"
#include "JobWorker.h"
#include <thread>

namespace Jobs
{
	JobWorker::JobWorker()
		: JobWorker(nullptr, 0)
	{
	}

	JobWorker::JobWorker(JobManager* Manager, uint32_t WorkerId)
		: _Running(false), _Manager(Manager), _WorkerId(WorkerId), _Executing(false)
	{
	}

	void JobWorker::Run()
	{
		_Running = true;
		while (_Running)
			this->TryRunTask();
	}

	void JobWorker::Stop()
	{
		_Running = false;
	}

	bool JobWorker::IsExecuting()
	{
		return (this->_Executing == true);
	}

	void JobWorker::TryRunTask()
	{
		auto Job = GetJob();
		if (Job != nullptr)
		{
			this->_Executing = true;
			Job->Execute();
			
			if (!Job->IsAwaiter())
				delete Job;
			this->_Executing = false;
		}
		else
		{
			std::this_thread::yield();
		}
	}
}
