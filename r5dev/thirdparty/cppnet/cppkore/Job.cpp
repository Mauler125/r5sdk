#include "stdafx.h"
#include "Job.h"

namespace Jobs
{
	Job::Job()
		: Job(nullptr, nullptr, true)
	{
	}

	Job::Job(std::function<void()> Task, std::function<void()> OnFinished, bool IsAwaiter)
		: _Task(std::move(Task)), _Finished(std::move(OnFinished)), _IsFinished(false), _IsAwaiter(IsAwaiter)
	{
	}

	void Job::Execute()
	{
		if (_Task != nullptr)
			_Task();
		if (_Finished != nullptr)
			_Finished();

		this->_IsFinished = true;
	}

	void Job::SetIsFinished()
	{
		this->_IsFinished = true;
	}

	bool Job::IsFinished()
	{
		return (this->_IsFinished == true);
	}

	bool Job::IsAwaiter()
	{
		return (this->_IsAwaiter == true);
	}
}
