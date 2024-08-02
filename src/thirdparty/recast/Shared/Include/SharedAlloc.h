//
// Copyright (c) 2009-2010 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#ifndef RECASTDETOURALLOC_H
#define RECASTDETOURALLOC_H

#include "SharedAssert.h"

/// Provides hint values to the memory allocator on how long the
/// memory is expected to be used.
enum rdAllocHint
{
	RD_ALLOC_PERM,		///< Memory will persist after a function call.
	RD_ALLOC_TEMP		///< Memory used temporarily within a function.
};

/// A memory allocation function.
//  @param[in]		size			The size, in bytes of memory, to allocate.
//  @param[in]		rdAllocHint	A hint to the allocator on how long the memory is expected to be in use.
//  @return A pointer to the beginning of the allocated memory block, or null if the allocation failed.
///  @see rdAllocSetCustom
typedef void* (rdAllocFunc)(size_t size, rdAllocHint hint);

/// A memory deallocation function.
///  @param[in]		ptr		A pointer to a memory block previously allocated using #rdAllocFunc.
/// @see rdAllocSetCustom
typedef void (rdFreeFunc)(void* ptr);

/// Sets the base custom allocation functions to be used by Recast & Detour.
///  @param[in]		allocFunc	The memory allocation function to be used by #rdAlloc
///  @param[in]		freeFunc	The memory de-allocation function to be used by #rdFree
///  
/// @see rdAlloc, rdFree
void rdAllocSetCustom(rdAllocFunc *allocFunc, rdFreeFunc *freeFunc);

/// Allocates a memory block.
/// 
/// @param[in]		size	The size, in bytes of memory, to allocate.
/// @param[in]		hint	A hint to the allocator on how long the memory is expected to be in use.
/// @return A pointer to the beginning of the allocated memory block, or null if the allocation failed.
/// 
/// @see rdFree, rdAllocSetCustom
void* rdAlloc(size_t size, rdAllocHint hint);

/// Deallocates a memory block.  If @p ptr is NULL, this does nothing.
///
/// @warning This function leaves the value of @p ptr unchanged.  So it still
/// points to the same (now invalid) location, and not to null.
/// 
/// @param[in]		ptr		A pointer to a memory block previously allocated using #rdAlloc.
/// 
/// @see rdAlloc, rdAllocSetCustom
void rdFree(void* ptr);

/// An implementation of operator new usable for placement new. The default one is part of STL (which we don't use).
/// rdNewTag is a dummy type used to differentiate our operator from the STL one, in case users import both Recast
/// & Detour and STL.
struct rdNewTag {};
inline void* operator new(size_t, const rdNewTag&, void* p) { return p; }
inline void operator delete(void*, const rdNewTag&, void*) {}

/// Signed to avoid warnings when comparing to int loop indexes, and common error with comparing to zero.
/// MSVC2010 has a bug where ssize_t is unsigned (!!!).
typedef intptr_t rdSizeType;
#define RD_SIZE_MAX INTPTR_MAX

/// Macros to hint to the compiler about the likeliest branch. Please add a benchmark that demonstrates a performance
/// improvement before introducing use cases.
#if defined(__GNUC__) || defined(__clang__)
#define rdLikely(x) __builtin_expect((x), true)
#define rdUnlikely(x) __builtin_expect((x), false)
#else
#define rdLikely(x) (x)
#define rdUnlikely(x) (x)
#endif

/// Variable-sized storage type. Mimics the interface of std::vector<T> with some notable differences:
///  * Uses rdAlloc()/rdFree() to handle storage.
///  * No support for a custom allocator.
///  * Uses signed size instead of size_t to avoid warnings in for loops: "for (int i = 0; i < foo.size(); i++)"
///  * Omits methods of limited utility: insert/erase, (bad performance), at (we don't use exceptions), operator=.
///  * assign() and the pre-sizing constructor follow C++11 semantics -- they don't construct a temporary if no value is provided.
///  * push_back() and resize() support adding values from the current vector. Range-based constructors and assign(begin, end) do not.
///  * No specialization for bool.
template <typename T, rdAllocHint H>
class rdVectorBase {
	rdSizeType m_size;
	rdSizeType m_cap;
	T* m_data;
	// Constructs a T at the give address with either the copy constructor or the default.
	static void construct(T* p, const T& v) { ::new(rdNewTag(), (void*)p) T(v); }
	static void construct(T* p) { ::new(rdNewTag(), (void*)p) T; }
	static void construct_range(T* begin, T* end);
	static void construct_range(T* begin, T* end, const T& value);
	static void copy_range(T* dst, const T* begin, const T* end);
	void destroy_range(rdSizeType begin, rdSizeType end);
	// Creates an array of the given size, copies all of this vector's data into it, and returns it.
	T* allocate_and_copy(rdSizeType size);
	void resize_impl(rdSizeType size, const T* value);
	// Requires: min_capacity > m_cap.
	rdSizeType get_new_capacity(rdSizeType min_capacity);
 public:
	typedef rdSizeType size_type;
	typedef T value_type;

	rdVectorBase() : m_size(0), m_cap(0), m_data(0) {}
	rdVectorBase(const rdVectorBase<T, H>& other) : m_size(0), m_cap(0), m_data(0) { assign(other.begin(), other.end()); }
	explicit rdVectorBase(rdSizeType count) : m_size(0), m_cap(0), m_data(0) { resize(count); }
	rdVectorBase(rdSizeType count, const T& value) : m_size(0), m_cap(0), m_data(0) { resize(count, value); }
	rdVectorBase(const T* begin, const T* end) : m_size(0), m_cap(0), m_data(0) { assign(begin, end); }
	~rdVectorBase() { destroy_range(0, m_size); rdFree(m_data); }

	// Unlike in std::vector, we return a bool to indicate whether the alloc was successful.
	bool reserve(rdSizeType size);

	void assign(rdSizeType count, const T& value) { clear(); resize(count, value); }
	void assign(const T* begin, const T* end);

	void resize(rdSizeType size) { resize_impl(size, NULL); }
	void resize(rdSizeType size, const T& value) { resize_impl(size, &value); }
	// Not implemented as resize(0) because resize requires T to be default-constructible.
	void clear() { destroy_range(0, m_size); m_size = 0; }

	void push_back(const T& value);
	void pop_back() { rdAssert(m_size > 0); back().~T(); m_size--; }

	rdSizeType size() const { return m_size; }
	rdSizeType capacity() const { return m_cap; }
	bool empty() const { return size() == 0; }

	const T& operator[](rdSizeType i) const { rdAssert(i >= 0 && i < m_size); return m_data[i]; }
	T& operator[](rdSizeType i) { rdAssert(i >= 0 && i < m_size); return m_data[i]; }

	const T& front() const { rdAssert(m_size); return m_data[0]; }
	T& front() { rdAssert(m_size); return m_data[0]; }
	const T& back() const { rdAssert(m_size); return m_data[m_size - 1]; }
	T& back() { rdAssert(m_size); return m_data[m_size - 1]; }
	const T* data() const { return m_data; }
	T* data() { return m_data; }

	T* begin() { return m_data; }
	T* end() { return m_data + m_size; }
	const T* begin() const { return m_data; }
	const T* end() const { return m_data + m_size; }

	void swap(rdVectorBase<T, H>& other);

	// Explicitly deleted.
	rdVectorBase& operator=(const rdVectorBase<T, H>& other);
};

template<typename T, rdAllocHint H>
bool rdVectorBase<T, H>::reserve(rdSizeType count) {
	if (count <= m_cap) {
		return true;
	}
	T* new_data = allocate_and_copy(count);
	if (!new_data) {
	  return false;
	}
	destroy_range(0, m_size);
	rdFree(m_data);
	m_data = new_data;
	m_cap = count;
	return true;
}
template <typename T, rdAllocHint H>
T* rdVectorBase<T, H>::allocate_and_copy(rdSizeType size) {
	rdAssert(RD_SIZE_MAX / static_cast<rdSizeType>(sizeof(T)) >= size);
	T* new_data = static_cast<T*>(rdAlloc(sizeof(T) * size, H));
	if (new_data) {
		copy_range(new_data, m_data, m_data + m_size);
	}
	return new_data;
}
template <typename T, rdAllocHint H>
void rdVectorBase<T, H>::assign(const T* begin, const T* end) {
	clear();
	reserve(end - begin);
	m_size = end - begin;
	copy_range(m_data, begin, end);
}
template <typename T, rdAllocHint H>
void rdVectorBase<T, H>::push_back(const T& value) {
	// rdLikely increases performance by ~50% on BM_rdVector_PushPreallocated,
	// and by ~2-5% on BM_rdVector_Push.
	if (rdLikely(m_size < m_cap)) {
		construct(m_data + m_size++, value);
		return;
	}

	const rdSizeType new_cap = get_new_capacity(m_cap + 1);
	T* data = allocate_and_copy(new_cap);
	// construct between allocate and destroy+free in case value is
	// in this vector.
	construct(data + m_size, value);
	destroy_range(0, m_size);
	m_size++;
	m_cap = new_cap;
	rdFree(m_data);
	m_data = data;
}

template <typename T, rdAllocHint H>
rdSizeType rdVectorBase<T, H>::get_new_capacity(rdSizeType min_capacity) {
	rdAssert(min_capacity <= RD_SIZE_MAX);
	if (rdUnlikely(m_cap >= RD_SIZE_MAX / 2))
		return RD_SIZE_MAX;
	return 2 * m_cap > min_capacity ? 2 * m_cap : min_capacity;
}

template <typename T, rdAllocHint H>
void rdVectorBase<T, H>::resize_impl(rdSizeType size, const T* value) {
	if (size < m_size) {
		destroy_range(size, m_size);
		m_size = size;
	} else if (size > m_size) {
		if (size <= m_cap) {
			if (value) {
				construct_range(m_data + m_size, m_data + size, *value);
			} else {
				construct_range(m_data + m_size, m_data + size);
			}
			m_size = size;
		} else {
			const rdSizeType new_cap = get_new_capacity(size);
			T* new_data = allocate_and_copy(new_cap);
			// We defer deconstructing/freeing old data until after constructing
			// new elements in case "value" is there.
			if (value) {
				construct_range(new_data + m_size, new_data + size, *value);
			} else {
				construct_range(new_data + m_size, new_data + size);
			}
			destroy_range(0, m_size);
			rdFree(m_data);
			m_data = new_data;
			m_cap = new_cap;
			m_size = size;
		}
	}
}
template <typename T, rdAllocHint H>
void rdVectorBase<T, H>::swap(rdVectorBase<T, H>& other) {
	// TODO: Reorganize headers so we can use rcSwap here.
	rdSizeType tmp_cap = other.m_cap;
	rdSizeType tmp_size = other.m_size;
	T* tmp_data = other.m_data;

	other.m_cap = m_cap;
	other.m_size = m_size;
	other.m_data = m_data;

	m_cap = tmp_cap;
	m_size = tmp_size;
	m_data = tmp_data;
}
// static
template <typename T, rdAllocHint H>
void rdVectorBase<T, H>::construct_range(T* begin, T* end) {
	for (T* p = begin; p < end; p++) {
		construct(p);
	}
}
// static
template <typename T, rdAllocHint H>
void rdVectorBase<T, H>::construct_range(T* begin, T* end, const T& value) {
	for (T* p = begin; p < end; p++) {
		construct(p, value);
	}
}
// static
template <typename T, rdAllocHint H>
void rdVectorBase<T, H>::copy_range(T* dst, const T* begin, const T* end) {
	for (rdSizeType i = 0 ; i < end - begin; i++) {
		construct(dst + i, begin[i]);
	}
}
template <typename T, rdAllocHint H>
void rdVectorBase<T, H>::destroy_range(rdSizeType begin, rdSizeType end) {
	for (rdSizeType i = begin; i < end; i++) {
		m_data[i].~T();
	}
}

template <typename T>
class rdTempVector : public rdVectorBase<T, RD_ALLOC_TEMP> {
	typedef rdVectorBase<T, RD_ALLOC_TEMP> Base;
public:
	rdTempVector() : Base() {}
	explicit rdTempVector(rdSizeType size) : Base(size) {}
	rdTempVector(rdSizeType size, const T& value) : Base(size, value) {}
	rdTempVector(const rdTempVector<T>& other) : Base(other) {}
	rdTempVector(const T* begin, const T* end) : Base(begin, end) {}
};
template <typename T>
class rdPermVector : public rdVectorBase<T, RD_ALLOC_PERM> {
	typedef rdVectorBase<T, RD_ALLOC_PERM> Base;
public:
	rdPermVector() : Base() {}
	explicit rdPermVector(rdSizeType size) : Base(size) {}
	rdPermVector(rdSizeType size, const T& value) : Base(size, value) {}
	rdPermVector(const rdPermVector<T>& other) : Base(other) {}
	rdPermVector(const T* begin, const T* end) : Base(begin, end) {}
};


/// Legacy class. Prefer rdVector<int>.
class rdIntArray
{
	rdTempVector<int> m_impl;
public:
	rdIntArray() {}
	rdIntArray(int n) : m_impl(n, 0) {}
	void push(int item) { m_impl.push_back(item); }
	void resize(int size) { m_impl.resize(size); }
	void clear() { m_impl.clear(); }
	int pop()
	{
		int v = m_impl.back();
		m_impl.pop_back();
		return v;
	}
	int size() const { return static_cast<int>(m_impl.size()); }
	int* data() { return m_impl.data(); }
	const int* data() const { return m_impl.data(); }
	int& operator[](int index) { return m_impl[index]; }
	int operator[](int index) const { return m_impl[index]; }
};

/// A simple helper class used to delete an array when it goes out of scope.
/// @note This class is rarely if ever used by the end user.
template<class T> class rdScopedDelete
{
	T* ptr;
public:

	/// Constructs an instance with a null pointer.
	inline rdScopedDelete() : ptr(0) {}

	/// Constructs an instance with the specified pointer.
	///  @param[in]		p	An pointer to an allocated array.
	inline rdScopedDelete(T* p) : ptr(p) {}
	inline ~rdScopedDelete() { rdFree(ptr); }

	/// The root array pointer.
	///  @return The root array pointer.
	inline operator T*() { return ptr; }
	
private:
	// Explicitly disabled copy constructor and copy assignment operator.
	rdScopedDelete(const rdScopedDelete&);
	rdScopedDelete& operator=(const rdScopedDelete&);
};

#endif // RECASTDETOURALLOC_H
