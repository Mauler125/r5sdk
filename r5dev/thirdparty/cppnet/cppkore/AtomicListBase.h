#pragma once

#include <atomic>
#include <thread>
#include <mutex>
#include "ListBase.h"

template<typename T>
class AtomicListBase
{
public:
	AtomicListBase();
	~AtomicListBase() = default;

	void Enqueue(T& Item);
	bool Dequeue(T& Item);

	bool IsEmpty() const;
	uint32_t Count() const;

private:
	List<T> _List;

	std::atomic<uint32_t> _SyncCount;
	std::mutex _SyncContext;
};

template<typename T>
inline AtomicListBase<T>::AtomicListBase()
	: _SyncCount(0)
{
}

template<typename T>
inline void AtomicListBase<T>::Enqueue(T& Item)
{
	{
		std::lock_guard lock(this->_SyncContext);
		this->_List.EmplaceBack(Item);
	}

	this->_SyncCount++;
}

template<typename T>
inline bool AtomicListBase<T>::Dequeue(T& Item)
{
	{
		std::lock_guard lock(this->_SyncContext);
		const auto Length = this->_List.Count();

		if (Length > 0)
		{
			Item = this->_List[Length - 1];
			this->_List.RemoveAt(Length - 1);

			this->_SyncCount--;
			return true;
		}
	}

	return false;
}

template<typename T>
inline bool AtomicListBase<T>::IsEmpty() const
{
	return (this->_SyncCount == 0);
}

template<typename T>
inline uint32_t AtomicListBase<T>::Count() const
{
	return this->_SyncCount;
}
