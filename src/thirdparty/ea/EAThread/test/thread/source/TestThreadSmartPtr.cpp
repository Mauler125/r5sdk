///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "TestThread.h"
#include <EATest/EATest.h>
#include <eathread/eathread_thread.h>
#include <eathread/shared_ptr_mt.h>
#include <eathread/shared_array_mt.h>
#include <eathread/eathread_atomic.h>  // required to test the c11 atomics macros

using namespace EA::Thread;


struct TestClass
{
	int x;
};


static bool IsTrue(bool b)
{
	return b;
}

static bool IsFalse(bool b)
{
	return !b;
}


namespace not_eastl
{
	// Stub of std::shared_ptr implementation used to exercise the C11 atomic
	// macro expansion.  This was causing issues because the function names of
	// the free standing eastl::shared_ptr functions and the C11 atomics
	// functions collided.  Since the C11 atomics were implemented as macros we
	// can not utilize any scoping so we are forced to prevent the problem
	// polluting the global namespace.
	template <typename T> class shared_ptr { };
	template <typename T> inline bool atomic_is_lock_free(const shared_ptr<T>*) { return false; }
	template <typename T> inline shared_ptr<T> atomic_load(const shared_ptr<T>* pSharedPtr) { return *pSharedPtr; }
	template <typename T> inline shared_ptr<T> atomic_load_explicit(const shared_ptr<T>* pSharedPtr, ...) { return *pSharedPtr; }
	template <typename T> inline void atomic_store(shared_ptr<T>* pSharedPtrA, shared_ptr<T> sharedPtrB) {}
	template <typename T> inline void atomic_store_explicit(shared_ptr<T>* pSharedPtrA, shared_ptr<T> sharedPtrB, ...) {}
	template <typename T> shared_ptr<T> atomic_exchange(shared_ptr<T>* pSharedPtrA, shared_ptr<T> sharedPtrB) { return sharedPtrB; }
	template <typename T> inline shared_ptr<T> atomic_exchange_explicit(shared_ptr<T>* pSharedPtrA, shared_ptr<T> sharedPtrB, ...){ return *pSharedPtrA; }
	template <typename T> bool atomic_compare_exchange_strong(shared_ptr<T>* pSharedPtr, shared_ptr<T>* pSharedPtrCondition, shared_ptr<T> sharedPtrNew) { return false; }
	template <typename T> inline bool atomic_compare_exchange_weak(shared_ptr<T>* pSharedPtr, shared_ptr<T>* pSharedPtrCondition, shared_ptr<T> sharedPtrNew) { return false; }
	template <typename T> inline bool atomic_compare_exchange_strong_explicit(shared_ptr<T>* pSharedPtr, shared_ptr<T>* pSharedPtrCondition, shared_ptr<T> sharedPtrNew, ...) { return  false; }
	template <typename T> inline bool atomic_compare_exchange_weak_explicit(shared_ptr<T>* pSharedPtr, shared_ptr<T>* pSharedPtrCondition, shared_ptr<T> sharedPtrNew, ...) { return false; }
}


int TestThreadSmartPtr()
{
	int nErrorCount(0);

	// C11 atomics & eastl::shared_ptr name collision tests.
	{
		// If you are seeing compiler errors regarding C11 atomic macro
		// expansions here look at header file eathread_atomic_android_c11.h.
		// Ensure C11 atomic macros are being undefined properly.  We hit this
		// problem on android-gcc config.
		using namespace not_eastl;
		shared_ptr<int> ptr1, ptr2, ptr3;

		atomic_is_lock_free(&ptr1);
		atomic_load(&ptr1);
		atomic_load_explicit(&ptr1);
		atomic_store(&ptr1, ptr2);
		atomic_store_explicit(&ptr1, ptr2);
		atomic_exchange(&ptr1, ptr2);
		atomic_exchange_explicit(&ptr1, ptr2);
		atomic_compare_exchange_strong(&ptr1, &ptr2, ptr3);
		atomic_compare_exchange_weak(&ptr1, &ptr2, ptr3);
		atomic_compare_exchange_strong_explicit(&ptr1, &ptr2, ptr3);
		atomic_compare_exchange_strong_explicit(&ptr1, &ptr2, ptr3);
	}

	{  // Basic single-threaded test.
		typedef shared_ptr_mt<TestClass> TestClassSharedPtr;

		// typedef T element_type;
		EATEST_VERIFY_MSG(sizeof(TestClassSharedPtr::element_type) > 0, "shared_ptr_mt failure.");

		// typedef T value_type;
		EATEST_VERIFY_MSG(sizeof(TestClassSharedPtr::value_type) > 0, "shared_ptr_mt failure");

		// explicit shared_ptr_mt(T* pValue = 0)
		TestClassSharedPtr pSharedPtr0;
		TestClassSharedPtr pSharedPtr1(new TestClass);
		TestClassSharedPtr pSharedPtr2(new TestClass);

		// shared_ptr_mt(shared_ptr_mt const& sharedPtr)
		TestClassSharedPtr pSharedPtr3(pSharedPtr1);

		// void lock() const
		// void unlock() const
		pSharedPtr0.lock();
		pSharedPtr0.unlock();

		// operator bool_() const
		EATEST_VERIFY_MSG(IsFalse(pSharedPtr0), "shared_ptr_mt failure");
		EATEST_VERIFY_MSG( IsTrue(pSharedPtr1), "shared_ptr_mt failure");

		// bool operator!() const
		EATEST_VERIFY_MSG( IsTrue(!pSharedPtr0), "shared_ptr_mt failure");
		EATEST_VERIFY_MSG(IsFalse(!pSharedPtr1), "shared_ptr_mt failure");

		// int use_count() const
		EATEST_VERIFY_MSG(pSharedPtr0.use_count() == 1, "shared_ptr_mt failure");
		EATEST_VERIFY_MSG(pSharedPtr1.use_count() == 2, "shared_ptr_mt failure");
		EATEST_VERIFY_MSG(pSharedPtr2.use_count() == 1, "shared_ptr_mt failure");
		EATEST_VERIFY_MSG(pSharedPtr3.use_count() == 2, "shared_ptr_mt failure");

		// bool unique() const
		EATEST_VERIFY_MSG( pSharedPtr0.unique(), "shared_ptr_mt failure");
		EATEST_VERIFY_MSG(!pSharedPtr1.unique(), "shared_ptr_mt failure");
		EATEST_VERIFY_MSG( pSharedPtr2.unique(), "shared_ptr_mt failure");
		EATEST_VERIFY_MSG(!pSharedPtr3.unique(), "shared_ptr_mt failure");

		// shared_ptr_mt& operator=(shared_ptr_mt const& sharedPtr)
		pSharedPtr3 = pSharedPtr2;

		EATEST_VERIFY_MSG(pSharedPtr1.use_count() == 1, "shared_ptr_mt failure");
		EATEST_VERIFY_MSG(pSharedPtr2.use_count() == 2, "shared_ptr_mt failure");
		EATEST_VERIFY_MSG(pSharedPtr3.use_count() == 2, "shared_ptr_mt failure");

		// void reset(T* pValue = 0)
		pSharedPtr2.reset();

		EATEST_VERIFY_MSG(IsFalse(pSharedPtr2), "shared_ptr_mt failure");
		EATEST_VERIFY_MSG(pSharedPtr1.use_count() == 1, "shared_ptr_mt failure");
		EATEST_VERIFY_MSG(pSharedPtr2.use_count() == 1, "shared_ptr_mt failure");
		EATEST_VERIFY_MSG(pSharedPtr3.use_count() == 1, "shared_ptr_mt failure");

		pSharedPtr3.reset(new TestClass);

		EATEST_VERIFY_MSG(IsTrue(pSharedPtr3), "shared_ptr_mt failure");
		EATEST_VERIFY_MSG(pSharedPtr3.use_count() == 1, "shared_ptr_mt failure");

		// T* get() const
		TestClass* pA = pSharedPtr1.get();
		TestClass* pB = pSharedPtr3.get();

		// template<class T>
		// inline T* get_pointer(const shared_ptr_mt<T>& sharedPtr)
		EATEST_VERIFY_MSG(get_pointer(pSharedPtr1) == pA, "shared_ptr_mt failure");
		EATEST_VERIFY_MSG(get_pointer(pSharedPtr3) == pB, "shared_ptr_mt failure");

		// void swap(shared_ptr_mt<T>& sharedPtr)
		pSharedPtr1.swap(pSharedPtr3);

		EATEST_VERIFY_MSG(pSharedPtr1.get() == pB, "shared_ptr_mt failure");
		EATEST_VERIFY_MSG(pSharedPtr3.get() == pA, "shared_ptr_mt failure");

		// T& operator*() const
		(*pSharedPtr1).x = 37;

		EATEST_VERIFY_MSG(pSharedPtr1.get()->x == 37, "shared_ptr_mt failure");

		// T* operator->() const
		pSharedPtr1->x = 73;

		EATEST_VERIFY_MSG(pSharedPtr1.get()->x == 73, "shared_ptr_mt failure");

		// template<class T>
		// inline void swap(shared_ptr_mt<T>& sharedPtr1, shared_ptr_mt<T>& sharedPtr2)
		swap(pSharedPtr1, pSharedPtr2);

		EATEST_VERIFY_MSG(pSharedPtr2.get()->x == 73, "shared_ptr_mt failure");

		// template<class T, class U>
		// inline bool operator==(const shared_ptr_mt<T>& sharedPtr1, const shared_ptr_mt<U>& sharedPtr2)
		bool bEqual = pSharedPtr1 == pSharedPtr2;

		EATEST_VERIFY_MSG(!bEqual, "shared_ptr_mt failure");

		// template<class T, class U>
		// inline bool operator!=(const shared_ptr_mt<T>& sharedPtr1, const shared_ptr_mt<U>& sharedPtr2)
		bool bNotEqual = pSharedPtr1 != pSharedPtr2;

		EATEST_VERIFY_MSG(bNotEqual, "shared_ptr_mt failure");

		// template<class T, class U>
		// inline bool operator<(const shared_ptr_mt<T>& sharedPtr1, const shared_ptr_mt<U>& sharedPtr2)
		bool bLessA = pSharedPtr1 < pSharedPtr2;
		bool bLessB = pSharedPtr2 < pSharedPtr1;

		EATEST_VERIFY_MSG(bLessA || bLessB, "shared_ptr_mt failure");
	}


	{
		typedef shared_array_mt<TestClass> TestClassSharedArray;

		TestClassSharedArray pSharedArray(new TestClass[5]);
		TestClassSharedArray pSharedArray2(pSharedArray);

		// To do: Implement the tests above.
	}

	return nErrorCount;
}












