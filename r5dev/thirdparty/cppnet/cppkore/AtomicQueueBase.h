#pragma once

#include <atomic>
#include <mutex>
#include "ListBase.h"

struct AtomicSyncContext
{
	std::atomic<uint32_t> _declspec(align(std::hardware_constructive_interference_size)) Head;
	std::atomic<uint32_t> _declspec(align(std::hardware_constructive_interference_size)) Tail;
};

template<typename T>
class AtomicQueueBase
{
public:
	AtomicQueueBase();
	~AtomicQueueBase() = default;

	bool Enqueue(T& Item, uint32_t Count = 1);
	bool Dequeue(T& Item, uint32_t Count = 1);

    bool IsEmpty() const;
    uint32_t Count() const;

private:
	AtomicSyncContext _Writer;
	AtomicSyncContext _Reader;

	constexpr static uint32_t BufferSize = (2 << 10);
	constexpr static uint32_t Mask = BufferSize - 1;

	std::atomic_flag _SpinLock;
	T alignas(std::hardware_constructive_interference_size) _Buffer[BufferSize];
};

template<typename T>
inline AtomicQueueBase<T>::AtomicQueueBase()
    : _Writer{ 0, 0 }, _Reader{ 0, 0 }, _Buffer{}, _SpinLock()
{
}

template<typename T>
inline bool AtomicQueueBase<T>::Enqueue(T& Item, uint32_t Count)
{
    uint32_t NextHead, EndTail, NewHead;

    // The queue can fail to insert if it's already full, so you must check the result..
    bool Success = false;
    do {
        NextHead = _Writer.Head.load(std::memory_order_acquire);
        EndTail = _Reader.Tail.load(std::memory_order_acquire);

        // Check if queue is full, yield time slice to other threads
        if ((NextHead - EndTail + 1) > Mask)
            return false;

        NewHead = NextHead + Count;
        Success = _Writer.Head.compare_exchange_weak(NextHead, NewHead, std::memory_order_release);
    } while (!Success);
    
    _Buffer[NextHead & Mask] = Item;

    std::atomic_thread_fence(std::memory_order_release);
    while (_Writer.Tail.load(std::memory_order_acquire) != NextHead)
    {
        // Spin lock wait
        while (_SpinLock.test_and_set(std::memory_order_acquire));
    }

    // Set the value and unlock the spin
    _Writer.Tail.store(NewHead, std::memory_order_release);
    _SpinLock.clear(std::memory_order_release);

    return true;
}

template<typename T>
inline bool AtomicQueueBase<T>::Dequeue(T& Item, uint32_t Count)
{
    uint32_t Head, Tail, Next;

    bool Success = false;
    do {
        Tail = _Reader.Head.load(std::memory_order_acquire);
        Head = _Writer.Tail.load(std::memory_order_acquire);

        // Check if the queue is empty, in that case, no result
        if (Head == Tail)
            return false;

        Next = Tail + Count;
        Success = _Reader.Head.compare_exchange_weak(Tail, Next, std::memory_order_release);
    } while (!Success);

    Item = _Buffer[Tail & Mask];

    std::atomic_thread_fence(std::memory_order_acquire);
    while (_Reader.Tail.load(std::memory_order_acquire) != Tail)
    {
        // Spin lock wait
        while (_SpinLock.test_and_set(std::memory_order_acquire));
    }

    _Reader.Tail.store(Next, std::memory_order_release);
    _SpinLock.clear(std::memory_order_release);

    return true;
}

template<typename T>
inline bool AtomicQueueBase<T>::IsEmpty() const
{
    uint32_t Tail = _Reader.Head.load(std::memory_order_acquire);
    uint32_t Head = _Writer.Tail.load(std::memory_order_acquire);

    // Check if the queue is empty, in that case, no result
    if (Head == Tail)
        return true;

    return false;
}

template<typename T>
inline uint32_t AtomicQueueBase<T>::Count() const
{
    // We need to subtract where we are writing to
    uint32_t Tail = _Reader.Head.load(std::memory_order_acquire);
    uint32_t Head = _Writer.Tail.load(std::memory_order_acquire);

    return (Head - Tail);
}
