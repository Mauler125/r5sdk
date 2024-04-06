///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "TestThread.h"
#include <EATest/EATest.h>
#include <eathread/eathread_atomic.h>
#include <eathread/eathread_thread.h>

EA_DISABLE_VC_WARNING(4265 4365 4836 4571 4625 4626 4628 4193 4127 4548)
#include <string.h>
EA_RESTORE_VC_WARNING()

#if defined(_MSC_VER)
	#pragma warning(disable: 4996) // This function or variable may be unsafe / deprecated.
#endif

#include <EASTL/numeric_limits.h>


using namespace EA::Thread;


#if EA_THREADS_AVAILABLE

const int kMaxConcurrentThreadCount = EATHREAD_MAX_CONCURRENT_THREAD_COUNT;

struct AWorkData32
{
	volatile bool mbShouldQuit;
	AtomicInt32   mnAtomicInteger1;
	AtomicInt32   mnAtomicInteger2;
	AtomicInt32   mnErrorCount;

	AWorkData32() : mbShouldQuit(false),
					mnAtomicInteger1(0), mnAtomicInteger2(0), 
					mnErrorCount(0) {}
};



static intptr_t Atomic32TestThreadFunction1(void* pvWorkData)
{
	int            nErrorCount = 0;
	AWorkData32*   pWorkData   = (AWorkData32*)pvWorkData;
	const ThreadId threadId    = GetThreadId();

	EA::UnitTest::ReportVerbosity(1, "Atomic test function 1 created, thread id %s\n", EAThreadThreadIdToString(threadId));

	// Do a series of operations, the final result of which is zero.
	while(!pWorkData->mbShouldQuit)
	{
		++pWorkData->mnAtomicInteger1;
		++pWorkData->mnAtomicInteger2;
		--pWorkData->mnAtomicInteger1;
		--pWorkData->mnAtomicInteger2;
		pWorkData->mnAtomicInteger1 += 5;
		pWorkData->mnAtomicInteger2 += 5;
		pWorkData->mnAtomicInteger1 -= 5;
		pWorkData->mnAtomicInteger2 -= 5;
		pWorkData->mnAtomicInteger1++;
		pWorkData->mnAtomicInteger2++;
		pWorkData->mnAtomicInteger1--;
		pWorkData->mnAtomicInteger2--;
		ThreadCooperativeYield();
	}

	pWorkData->mnErrorCount += nErrorCount;

	EA::UnitTest::ReportVerbosity(1, "Atomic test function 1 exiting, thread id %s\n", EAThreadThreadIdToString(threadId));
	return 0;
}


static intptr_t Atomic32TestThreadFunction2(void* pvWorkData)
{
	int            nErrorCount = 0;
	AWorkData32*   pWorkData   = (AWorkData32*)pvWorkData;
	const ThreadId threadId = GetThreadId();
	ThreadUniqueId threadUniqueId;
	EAThreadGetUniqueId(threadUniqueId);
	int32_t        threadUniqueId32 = (int32_t)threadUniqueId;

	EA::UnitTest::ReportVerbosity(1, "Atomic test function 2 created, thread id %s\n", EAThreadThreadIdToString(threadId));

	// Test the SetValueConditional function. We basically create a spinlock here.
	while(!pWorkData->mbShouldQuit)
	{
		if(pWorkData->mnAtomicInteger1.SetValueConditional(threadUniqueId32, 0x11223344))
		{
			EATEST_VERIFY_MSG(pWorkData->mnAtomicInteger1 == threadUniqueId32, "AtomicInt SetValueConditional failure.");
			pWorkData->mnAtomicInteger1 = 0x11223344;
		}

		ThreadCooperativeYield();
	}

	pWorkData->mnErrorCount += nErrorCount;

	EA::UnitTest::ReportVerbosity(1, "Atomic test function 2 exiting, thread id %s\n", EAThreadThreadIdToString(threadId));
	return 0;
}


struct AWorkData64
{
	volatile bool mbShouldQuit;
	AtomicInt64   mnAtomicInteger1;
	AtomicInt64   mnAtomicInteger2;
	AtomicInt64   mnErrorCount;

	AWorkData64() : mbShouldQuit(false),
					mnAtomicInteger1(0), mnAtomicInteger2(0), 
					mnErrorCount(0) {}
};


static intptr_t Atomic64TestThreadFunction1(void* pvWorkData)
{
	int            nErrorCount = 0;
	AWorkData64*   pWorkData   = (AWorkData64*)pvWorkData;
	const ThreadId threadId    = GetThreadId();

	EA::UnitTest::ReportVerbosity(1, "Atomic64 test function 1 created, thread id %s\n", EAThreadThreadIdToString(threadId));

	// Do a series of operations, the final result of which is zero.
	while(!pWorkData->mbShouldQuit)
	{
		++pWorkData->mnAtomicInteger1;
		++pWorkData->mnAtomicInteger2;
		--pWorkData->mnAtomicInteger1;
		--pWorkData->mnAtomicInteger2;
		pWorkData->mnAtomicInteger1 += UINT64_C(0x0000000fffffffff);
		pWorkData->mnAtomicInteger2 += UINT64_C(0x0000000ffffffffe);
		pWorkData->mnAtomicInteger1 -= UINT64_C(0x0000000fffffffff);
		pWorkData->mnAtomicInteger2 -= UINT64_C(0x0000000ffffffffe);
		pWorkData->mnAtomicInteger1++;
		pWorkData->mnAtomicInteger2++;
		pWorkData->mnAtomicInteger1--;
		pWorkData->mnAtomicInteger2--;
		ThreadCooperativeYield();
	}

	pWorkData->mnErrorCount += nErrorCount;

	EA::UnitTest::ReportVerbosity(1, "Atomic64 test function 1 exiting, thread id %s\n", EAThreadThreadIdToString(threadId));
	return 0;
}


static intptr_t Atomic64TestThreadFunction2(void* pvWorkData)
{
	int            nErrorCount = 0;
	AWorkData64*   pWorkData   = (AWorkData64*)pvWorkData;
	const ThreadId threadId = GetThreadId();
	ThreadUniqueId threadUniqueId;
	EAThreadGetUniqueId(threadUniqueId);
	uint64_t       threadUnqueId64 = (uint64_t)threadUniqueId | UINT64_C(0xeeeeddddffffffff);

	EA::UnitTest::ReportVerbosity(1, "Atomic64 test function 2 created, thread id %s\n", EAThreadThreadIdToString(threadId));

	// Test the SetValueConditional function. We basically create a spinlock here.
	while(!pWorkData->mbShouldQuit)
	{
		if(pWorkData->mnAtomicInteger1.SetValueConditional(threadUnqueId64, 0x1122334455667788))
		{
			EATEST_VERIFY_MSG(pWorkData->mnAtomicInteger1 == static_cast<int64_t>(threadUnqueId64), "AtomicInt64 SetValueConditional failure.");
			pWorkData->mnAtomicInteger1.SetValue(0x1122334455667788);
		}

		ThreadCooperativeYield();
	}

	pWorkData->mnErrorCount += nErrorCount;

	EA::UnitTest::ReportVerbosity(1, "Atomic64 test function 2 exiting, thread id %s\n", EAThreadThreadIdToString(threadId));
	return 0;
}


static intptr_t Atomic64TestThreadFunction3(void* pvWorkData)
{
	int            nErrorCount = 0;
	AWorkData64*   pWorkData   = (AWorkData64*)pvWorkData;
	const ThreadId threadId    = GetThreadId();
	const uint64_t value0      = UINT64_C(0x0000000000000000);
	const uint64_t value1      = UINT64_C(0xffffffffffffffff);
	
	EA::UnitTest::ReportVerbosity(1, "Atomic64 test function 2 created, thread id %s\n", EAThreadThreadIdToString(threadId));

	// Test the SetValueConditional function.
	while(!pWorkData->mbShouldQuit)
	{
		pWorkData->mnAtomicInteger1.SetValueConditional(value0, value1);
		uint64_t currentValue = pWorkData->mnAtomicInteger1.GetValue();
		EATEST_VERIFY_MSG((currentValue == value0) || (currentValue == value1), "AtomicInt64 SetValueConditional failure.");
		
		pWorkData->mnAtomicInteger1.SetValueConditional(value1, value0);
		currentValue = pWorkData->mnAtomicInteger1.GetValue();
		EATEST_VERIFY_MSG((currentValue == value0) || (currentValue == value1), "AtomicInt64 SetValueConditional failure.");

		ThreadCooperativeYield();
	}

	pWorkData->mnErrorCount += nErrorCount;

	EA::UnitTest::ReportVerbosity(1, "Atomic64 test function 2 exiting, thread id %s\n", EAThreadThreadIdToString(threadId));
	return 0;
}

template<typename T>
int TestSimpleAtomicOps()
{
	int nErrorCount = 0; 
	bool result = false;

	alignas(16) T value = 0;
	alignas(16) T dest = 0;
	alignas(16) T conditionFail = 4;
	alignas(16) T conditionSucceed = 0;

	// AtomicGetValue
	dest = 3;
	value = AtomicGetValue(&dest);
	EATEST_VERIFY_MSG(value == 3, "AtomicGetValue failure\n");

	// AtomicSetValue
	value = AtomicSetValue(&dest, 4);
	EATEST_VERIFY_MSG(value == 3, "AtomicSetValue failure\n");
	value = AtomicGetValue(&dest);
	EATEST_VERIFY_MSG(value == 4, "AtomicSetValue failure\n");

	// AtomicFetchIncrement
	value = AtomicFetchIncrement(&dest);
	EATEST_VERIFY_MSG(value == 4, "AtomicFetchIncrement failure\n");
	value = AtomicGetValue(&dest);
	EATEST_VERIFY_MSG(value == 5, "AtomicFetchIncrement failure\n");

	// AtomicFetchDecrement
	value = AtomicFetchDecrement(&dest);
	EATEST_VERIFY_MSG(value == 5, "AtomicFetchDecrement failure\n");
	value = AtomicGetValue(&dest);
	EATEST_VERIFY_MSG(value == 4, "AtomicFetchDecrement failure\n");

	// AtomicFetchAdd
	value = AtomicFetchAdd(&dest, 3);
	EATEST_VERIFY_MSG(value == 4, "AtomicFetchAdd failure\n");
	value = AtomicGetValue(&dest);
	EATEST_VERIFY_MSG(value == 7, "AtomicFetchAdd failure\n");

	// AtomicFetchSub
	value = AtomicFetchSub(&dest, 3);
	EATEST_VERIFY_MSG(value == 7, "AtomicFetchSub failure\n");
	value = AtomicGetValue(&dest);
	EATEST_VERIFY_MSG(value == 4, "AtomicFetchSub failure\n");
	value = AtomicFetchSub(&dest, T(-3));
	EATEST_VERIFY_MSG(value == 4, "AtomicFetchSub failure\n");
	value = AtomicGetValue(&dest);
	EATEST_VERIFY_MSG(value == 7, "AtomicFetchSub failure\n");

	// AtomicFetchOr
	value = AtomicFetchOr(&dest, 8);
	EATEST_VERIFY_MSG(value == 7, "AtomicFetchOr failure\n");
	value = AtomicGetValue(&dest);
	EATEST_VERIFY_MSG(value == 15, "AtomicFetchOr failure\n");

	// AtomicFetchAnd
	value = AtomicFetchAnd(&dest, 3);
	EATEST_VERIFY_MSG(value == 15, "AtomicFetchAnd failure\n");
	value = AtomicGetValue(&dest);
	EATEST_VERIFY_MSG(value == 3, "AtomicFetchAnd failure\n");

	// AtomicFetchXor
	value = AtomicFetchXor(&dest, dest);
	EATEST_VERIFY_MSG(value == 3, "AtomicFetchXor failure\n");
	value = AtomicGetValue(&dest);
	EATEST_VERIFY_MSG(value == 0, "AtomicFetchXor failure\n");

	// AtomicFetchSwap
	value = AtomicFetchSwap(&dest, 5);
	EATEST_VERIFY_MSG(value == 0, "AtomicFetchSwap failure\n");
	value = AtomicGetValue(&dest);
	EATEST_VERIFY_MSG(value == 5, "AtomicFetchSwap failure\n");

	// AtomicSetValueConditional
	dest             = 0;
	value            = 1;
	conditionFail    = 4;
	conditionSucceed = 0;

	// Try to do conditional fetch swap which should fail
	value = EA::Thread::AtomicFetchSwapConditional(&dest, 1, conditionFail);
	EATEST_VERIFY_MSG(value != conditionFail, "AtomicFetchSwapConditional failure 0\n");
	EATEST_VERIFY_MSG(dest == 0, "AtomicFetchSwapConditional failure 1\n");

	// Try to do conditional fetch swap which should succeed
	value = EA::Thread::AtomicFetchSwapConditional(&dest, 1, conditionSucceed);
	EATEST_VERIFY_MSG(value == conditionSucceed, "AtomicFetchSwapConditional failure 2\n");
	EATEST_VERIFY_MSG(dest == 1, "AtomicFetchSwapConditional failure 3\n");

	// reset before the next test
	dest = 0;
	value = 1;

	// Try to do an update which should fail.
	result = EA::Thread::AtomicSetValueConditional(&dest, value, conditionFail);
	EATEST_VERIFY_MSG(!result, "AtomicSetValueConditional failure 0\n");
	EATEST_VERIFY_MSG(dest == 0, "AtomicSetValueConditional failure 1\n");

	// Try to do an update which should succeed.
	result = EA::Thread::AtomicSetValueConditional(&dest, value, conditionSucceed);
	EATEST_VERIFY_MSG(result, "AtomicSetValueConditional failure 2\n");
	EATEST_VERIFY_MSG(dest == 1, "AtomicSetValueConditional failure 3\n");

	return nErrorCount;
}

template<typename T>
inline int TestAtomicsSizeBoundaries()
{
	static_assert(eastl::is_floating_point<T>::value == false, "atomic floats not supported");

	int nErrorCount = 0;
	bool result = false;
	alignas(16) T value = 0, dest = 0;

	T max = eastl::numeric_limits<T>::max();
	T lowest = eastl::numeric_limits<T>::lowest();


	/// Test the max boundary 
	///
	value = AtomicSetValue(&dest, max);
	EATEST_VERIFY_MSG(value == 0, "max failure\n");
	value = AtomicGetValue(&dest);
	EATEST_VERIFY_MSG(value == max, "max failure\n");

	value = AtomicFetchIncrement(&dest);
	EATEST_VERIFY_MSG(value == max, "max failure\n");
	value = AtomicGetValue(&dest);
	EATEST_VERIFY_MSG(value == lowest, "max failure\n");

	value = AtomicFetchDecrement(&dest);
	EATEST_VERIFY_MSG(value == lowest, "max failure\n");
	value = AtomicGetValue(&dest);
	EATEST_VERIFY_MSG(value == max, "max failure\n");

	value = AtomicFetchAdd(&dest, 1);
	EATEST_VERIFY_MSG(value == max, "max failure\n");
	value = AtomicGetValue(&dest);
	EATEST_VERIFY_MSG(value == lowest, "max failure\n");

	value = AtomicFetchAnd(&dest, lowest);
	EATEST_VERIFY_MSG(value == lowest, "max failure\n");
	value = AtomicGetValue(&dest);
	EATEST_VERIFY_MSG(value == lowest, "max failure\n");

	value = AtomicFetchXor(&dest, lowest);
	EATEST_VERIFY_MSG(value == lowest, "max failure\n");
	value = AtomicGetValue(&dest);
	EATEST_VERIFY_MSG(value == 0, "max failure\n");

	value = AtomicFetchSwap(&dest, lowest);
	EATEST_VERIFY_MSG(value == 0, "max failure\n");
	value = AtomicGetValue(&dest);
	EATEST_VERIFY_MSG(value == lowest, "max failure\n");

	// reset to zero
	result = AtomicSetValueConditional(&dest, 0, lowest);
	EATEST_VERIFY_MSG(result, "max failure\n");
	value = AtomicGetValue(&dest);
	EATEST_VERIFY_MSG(value == 0, "max failure\n");
	


	/// Test the lowest boundary 
	///
	value = AtomicSetValue(&dest, lowest);
	EATEST_VERIFY_MSG(value == 0, "lowest failure\n");
	value = AtomicGetValue(&dest);
	EATEST_VERIFY_MSG(value == lowest, "lowest failure\n");

	// decrement the lowest to ensure we rollover to the highest value
	value = AtomicFetchDecrement(&dest);
	EATEST_VERIFY_MSG(value == lowest, "lowest failure\n");
	value = AtomicGetValue(&dest);
	EATEST_VERIFY_MSG(value == max, "lowest failure\n");


	return nErrorCount;
}

template<typename T>
inline int TestAtomicsConstFetch()
{
	int nErrorCount = 0;

	{
		alignas(16) const T value = 13;
		auto r = AtomicGetValue(&value);
		EATEST_VERIFY_MSG(r == value, "failure\n");
	}

	{
		struct Foo
		{
			Foo(uint32_t n) : baz(n) {}
			uint32_t getBaz() const { return AtomicGetValue(&this->baz); }
			uint32_t baz;
		};

		Foo foo(42);
		auto r = foo.getBaz();
		EATEST_VERIFY_MSG(r == 42, "failure\n");
	}

	return nErrorCount;
}

template<typename T>
int TestAtomicIntT()
{
	int nErrorCount = 0;

	AtomicInt<T> atomicInt = 0;

	++atomicInt;
	--atomicInt;
	atomicInt += 5;
	atomicInt -= 5;
	atomicInt++;
	atomicInt--;

	EATEST_VERIFY(atomicInt == 0);

	return nErrorCount;
}

template<typename T>
int TestNonMemberAtomics()
{
	int nErrorCount = 0;
	nErrorCount += TestSimpleAtomicOps<T>();
	nErrorCount += TestAtomicsSizeBoundaries<T>();
	nErrorCount += TestAtomicsConstFetch<T>();
	return nErrorCount;
}

#endif // #if EA_THREADS_AVAILABLE


int TestThreadAtomic()
{
	int nErrorCount(0);

	{ // Initial tests of 128 bit atomics
		#if EATHREAD_ATOMIC_128_SUPPORTED // This will be true only for 64+ bit platforms.

			// To consider: Use __int128_t on GCC for GCC >= 4.1

			EA_ALIGN(128) int64_t dest128[2]             = { 0, 0 };
			EA_ALIGN(128) int64_t value128[2]            = { 1, 2 };
			EA_ALIGN(128) int64_t condition128Fail[2]    = { 4, 5 };
			EA_ALIGN(128) int64_t condition128Succeed[2] = { 0, 0 };
			bool    result;

			// Try to do an update which should fail.
			result = EA::Thread::AtomicSetValueConditionall28(dest128, value128, condition128Fail);

			EATEST_VERIFY_MSG(!result, "AtomicSetValueConditional failure: result should have been false.\n");
			EATEST_VERIFY_F((dest128[0] == 0) && (dest128[1] == 0), "AtomicSetValueConditional failure: dest128[0]:%I64d dest128[1]:%I64d\n", dest128[0], dest128[1]);
			EATEST_VERIFY_F((value128[0] == 1) && (value128[1] == 2), "AtomicSetValueConditional failure: value128[0]:%I64d value128[1]:%I64d\n", value128[0], value128[1]);
			EATEST_VERIFY_F((condition128Fail[0] == 4) && (condition128Fail[1] == 5), "AtomicSetValueConditional failure: condition128Fail[0]:%I64d condition128Fail[1]:%I64d\n", condition128Fail[0], condition128Fail[1]);
			EATEST_VERIFY_F((condition128Succeed[0] == 0) && (condition128Succeed[1] == 0), "AtomicSetValueConditional failure: condition128Succeed[0]:%I64d condition128Succeed[1]:%I64d\n", condition128Succeed[0], condition128Succeed[1]);

			// Try to do an update which should succeed.
			// VC++ for VS2010 misgenerates the atomic code below in optimized builds, by passing what appears to be the wrong value for dest128 to AtomicSetValueConditional128. 
			// We added some diagnostic code and now the compiler does the right thing. I (Paul Pedriana) wonder if the problem is related to 
			// the alignment specification for dest, which must be 128 byte aligned for AtomicSetValueConditional128 (cmpxchg16b) to work.
			// The dest address is indeed being aligned to 16 bytes, so that's not the problem.
			EA::UnitTest::ReportVerbosity(1, "%p %p %p\n", dest128, value128, condition128Succeed);
			result = EA::Thread::AtomicSetValueConditionall28(dest128, value128, condition128Succeed);

			EATEST_VERIFY_MSG(result, "AtomicSetValueConditional failure: result should have been true.\n");
			EATEST_VERIFY_F((dest128[0] == 1) && (dest128[1] == 2), "AtomicSetValueConditional failure: dest128:%p dest128[0]:%I64d dest128[1]:%I64d\n", dest128, dest128[0], dest128[1]);
			EATEST_VERIFY_F((value128[0] == 1) && (value128[1] == 2), "AtomicSetValueConditional failure: value128:%p value128[0]:%I64d value128[1]:%I64d\n", value128, value128[0], value128[1]);
			EATEST_VERIFY_F((condition128Fail[0] == 4) && (condition128Fail[1] == 5), "AtomicSetValueConditional failure: condition128Fail:%p condition128Fail[0]:%I64d condition128Fail[1]:%I64d\n", condition128Fail, condition128Fail[0], condition128Fail[1]);
			EATEST_VERIFY_F((condition128Succeed[0] == 0) && (condition128Succeed[1] == 0), "AtomicSetValueConditional failure: condition128Succeed:%p condition128Succeed[0]:%I64d condition128Succeed[1]:%I64d\n", condition128Succeed, condition128Succeed[0], condition128Succeed[1]);


			#if defined(EA_COMPILER_GNUC)   // GCC defines __int128_t as a built-in type.

				__int128_t dest;
				__int128_t value;
				__int128_t conditionFail;
				__int128_t conditionSucceed;

				// AtomicGetValue
				dest = 3;
				value = AtomicGetValue(&dest);
				EATEST_VERIFY_MSG(value == 3, "AtomicGetValue[128] failure\n");

				// AtomicSetValue
				AtomicSetValue(&dest, 4);
				value = AtomicGetValue(&dest);
				EATEST_VERIFY_MSG(value == 4, "AtomicSetValue[128] failure\n");

				// AtomicIncrement
				value = AtomicIncrement(&dest);
				EATEST_VERIFY_MSG(value == 5, "AtomicIncrement[128] failure\n");
				value = AtomicGetValue(&dest);
				EATEST_VERIFY_MSG(value == 5, "AtomicIncrement[128] failure\n");

				// AtomicDecrement
				value = AtomicDecrement(&dest);
				EATEST_VERIFY_MSG(value == 4, "AtomicDecrement[128] failure\n");
				value = AtomicGetValue(&dest);
				EATEST_VERIFY_MSG(value == 4, "AtomicDecrement[128] failure\n");

				// AtomicAdd
				value = AtomicAdd(&dest, 3);
				EATEST_VERIFY_MSG(value == 7, "AtomicAdd[128] failure\n");
				value = AtomicGetValue(&dest);
				EATEST_VERIFY_MSG(value == 7, "AtomicAdd[128] failure\n");

				// AtomicOr
				value = AtomicOr(&dest, 8);
				EATEST_VERIFY_MSG(value == 15, "AtomicOr[128] failure\n");
				value = AtomicGetValue(&dest);
				EATEST_VERIFY_MSG(value == 15, "AtomicOr[128] failure\n");

				// AtomicAnd
				value = AtomicAnd(&dest, 3);
				EATEST_VERIFY_MSG(value == 3, "AtomicAnd[128] failure\n");
				value = AtomicGetValue(&dest);
				EATEST_VERIFY_MSG(value == 3, "AtomicAnd[128] failure\n");

				// AtomicXor
				value = AtomicXor(&dest, dest);
				EATEST_VERIFY_MSG(value == 0, "AtomicXor[128] failure\n");
				value = AtomicGetValue(&dest);
				EATEST_VERIFY_MSG(value == 0, "AtomicXor[128] failure\n");

				// AtomicSwap
				value = AtomicSwap(&dest, 5);
				EATEST_VERIFY_MSG(value == 0, "AtomicSwap[128] failure\n");
				value = AtomicGetValue(&dest);
				EATEST_VERIFY_MSG(value == 5, "AtomicSwap[128] failure\n");

				// AtomicSetValueConditional
				dest             = 0;
				value            = 1;
				conditionFail    = 4;
				conditionSucceed = 0;

				// Try to do an update which should fail.
				result = EA::Thread::AtomicSetValueConditional(&dest, value, conditionFail);

				EATEST_VERIFY_MSG(!result, "AtomicSetValueConditional failure 0\n");
				EATEST_VERIFY_MSG(dest == 0, "AtomicSetValueConditional failure 1\n");

				// Try to do an update which should succeed.
				result = EA::Thread::AtomicSetValueConditional(&dest, value, conditionSucceed);

				EATEST_VERIFY_MSG(result, "AtomicSetValueConditional failure 2\n");
				EATEST_VERIFY_MSG(dest == 1, "AtomicSetValueConditional failure 3\n");

				if(nErrorCount != 0)
					return nErrorCount;

			#endif

		#endif
	}

	{  // Basic single-threaded Atomic test.
		AtomicInt32  i32(1);
		AtomicUint32 u32(1);

		EATEST_VERIFY_MSG(i32.GetValue() == 1, "AtomicInt32 failure.");
		EATEST_VERIFY_MSG(u32.GetValue() == 1, "AtomicUint32 failure.");

		char buffer[64];
		sprintf(buffer, "%d %u", (signed int)i32.GetValue(), (unsigned int)u32.GetValue());
		EATEST_VERIFY_MSG(strcmp(buffer, "1 1") == 0, "AtomicInt32 failure.");

		// Copy ctor/operator=.
		AtomicInt32 i32CopyA(i32);
		AtomicInt32 i32CopyB(i32CopyA);
		i32CopyA = i32CopyB;

		sprintf(buffer, "%d %d", (signed int)i32CopyA.GetValue(), (signed int)i32CopyB.GetValue());
		EATEST_VERIFY_MSG(strcmp(buffer, "1 1") == 0, "AtomicInt32 failure.");

		// Test platforms that support 64 bits..
		AtomicInt64  i64(1);
		AtomicUint64 u64(1);

		sprintf(buffer, "%.0f %.0f", (double)i64.GetValue(), (double)u64.GetValue());
		EATEST_VERIFY_MSG(strcmp(buffer, "1 1") == 0, "AtomicInt64 failure.");

		// Copy ctor/operator=.
		AtomicInt64 i64CopyA(i64);
		AtomicInt64 i64CopyB(i64CopyA);
		i64CopyA = i64CopyB;

		sprintf(buffer, "%d %d", (signed int)i64CopyA.GetValue(), (signed int)i64CopyB.GetValue());
		EATEST_VERIFY_MSG(strcmp(buffer, "1 1") == 0, "AtomicInt64 failure.");
	
		bool result = i64.SetValueConditional(2, 99999); // This should not set the value to 2.
		EATEST_VERIFY_MSG(!result && (i64.GetValue() == 1), "AtomicInt64 failure.");
	
		i64.SetValueConditional(2, 1);     // This should set the value to 2.
		EATEST_VERIFY_MSG(!result && (i64.GetValue() == 2), "AtomicInt64 failure.");
	}

	{  // Basic single-threaded AtomicInt32 test.
		AtomicInt32 i(0); // Note that this assignment goes through AtomicInt32 operator=().
		AtomicInt32::ValueType x;

		EATEST_VERIFY_MSG(i == 0, "AtomicInt32 failure.");

		++i;
		i++;
		--i;
		i--;
		i += 7;
		i -= 3;
		EATEST_VERIFY_MSG(i == 4, "AtomicInt32 failure.");

		i = 2;
		x = i.GetValue();
		EATEST_VERIFY_MSG(x == 2, "AtomicInt32 failure.");

		i.Increment();
		i.Decrement();
		i.Add(5);
		i.Add(-2);
		EATEST_VERIFY_MSG(i == 5, "AtomicInt32 failure.");

		i.SetValue(6);
		EATEST_VERIFY_MSG(i == 6, "AtomicInt32 failure.");

		bool bWasEqualTo10000 = i.SetValueConditional(3, 10000);
		EATEST_VERIFY_MSG(!bWasEqualTo10000, "AtomicInt32 failure.");

		bool bWasEqualTo6 = i.SetValueConditional(3, 6);
		EATEST_VERIFY_MSG(bWasEqualTo6, "AtomicInt32 failure.");
	}

	{  // Verify pre-increment/post-increment works as intended.
		AtomicInt32            i32(0);
		AtomicInt32::ValueType x32;

		// ValueType SetValue(ValueType n)
		// Safely sets a new value. Returns the old value.
		x32 = i32.SetValue(1);
		EATEST_VERIFY_MSG(x32 == 0, "AtomicInt return value failure.");

		// ValueType Increment()
		// Safely increments the value. Returns the new value.
		x32 = i32.Increment();
		EATEST_VERIFY_MSG(x32 == 2, "AtomicInt return value failure.");

		// ValueType Decrement()
		// Safely decrements the value. Returns the new value.
		x32 = i32.Decrement();
		EATEST_VERIFY_MSG(x32 == 1, "AtomicInt return value failure.");

		// ValueType Add(ValueType n)
		// Safely adds a value, which can be negative. Returns the new value.
		x32 = i32.Add(35);
		EATEST_VERIFY_MSG(x32 == 36, "AtomicInt return value failure.");

		// ValueType operator=(ValueType n)
		// Safely assigns the value. Returns the new value.
		x32 = (i32 = 17);
		EATEST_VERIFY_MSG(x32 == 17, "AtomicInt return value failure.");

		// ValueType operator+=(ValueType n)
		// Safely adds a value, which can be negative. Returns the new value.
		x32 = (i32 += 3);
		EATEST_VERIFY_MSG(x32 == 20, "AtomicInt return value failure.");

		// ValueType operator-=(ValueType n)
		// Safely subtracts a value, which can be negative. Returns the new value.
		x32 = (i32 -= 6);
		EATEST_VERIFY_MSG(x32 == 14, "AtomicInt return value failure.");

		// ValueType operator++()
		// pre-increment operator++
		x32 = ++i32;
		EATEST_VERIFY_MSG(x32 == 15, "AtomicInt return value failure.");
		EATEST_VERIFY_MSG(i32 == 15, "AtomicInt return value failure.");

		// ValueType operator++(int)
		// post-increment operator++
		x32 = i32++;
		EATEST_VERIFY_MSG(x32 == 15, "AtomicInt return value failure.");
		EATEST_VERIFY_MSG(i32 == 16, "AtomicInt return value failure.");

		// ValueType operator--()
		// pre-increment operator--
		x32 = --i32;
		EATEST_VERIFY_MSG(x32 == 15, "AtomicInt return value failure.");
		EATEST_VERIFY_MSG(i32 == 15, "AtomicInt return value failure.");

		// ValueType operator--(int)
		// post-increment operator--
		x32 = i32--;
		EATEST_VERIFY_MSG(x32 == 15, "AtomicInt return value failure.");
		EATEST_VERIFY_MSG(i32 == 14, "AtomicInt return value failure.");
	}


	{  // Verify pre-increment/post-increment works as intended.
		AtomicInt64            i64(0);
		AtomicInt64::ValueType x64;

		// ValueType SetValue(ValueType n)
		// Safely sets a new value. Returns the old value.
		x64 = i64.SetValue(1);
		EATEST_VERIFY_MSG(x64 == 0, "AtomicInt return value failure.");

		// ValueType Increment()
		// Safely increments the value. Returns the new value.
		x64 = i64.Increment();
		EATEST_VERIFY_MSG(x64 == 2, "AtomicInt return value failure.");

		// ValueType Decrement()
		// Safely decrements the value. Returns the new value.
		x64 = i64.Decrement();
		EATEST_VERIFY_MSG(x64 == 1, "AtomicInt return value failure.");

		// ValueType Add(ValueType n)
		// Safely adds a value, which can be negative. Returns the new value.
		x64 = i64.Add(35);
		EATEST_VERIFY_MSG(x64 == 36, "AtomicInt return value failure.");

		// ValueType operator=(ValueType n)
		// Safely assigns the value. Returns the new value.
		x64 = (i64 = 17);
		EATEST_VERIFY_MSG(x64 == 17, "AtomicInt return value failure.");

		// ValueType operator+=(ValueType n)
		// Safely adds a value, which can be negative. Returns the new value.
		x64 = (i64 += 3);
		EATEST_VERIFY_MSG(x64 == 20, "AtomicInt return value failure.");

		// ValueType operator-=(ValueType n)
		// Safely subtracts a value, which can be negative. Returns the new value.
		x64 = (i64 -= 6);
		EATEST_VERIFY_MSG(x64 == 14, "AtomicInt return value failure.");

		// ValueType operator++()
		// pre-increment operator++
		x64 = ++i64;
		EATEST_VERIFY_MSG(x64 == 15, "AtomicInt return value failure.");
		EATEST_VERIFY_MSG(i64 == 15, "AtomicInt return value failure.");

		// ValueType operator++(int)
		// post-increment operator++
		x64 = i64++;
		EATEST_VERIFY_MSG(x64 == 15, "AtomicInt return value failure.");
		EATEST_VERIFY_MSG(i64 == 16, "AtomicInt return value failure.");

		// ValueType operator--()
		// pre-increment operator--
		x64 = --i64;
		EATEST_VERIFY_MSG(x64 == 15, "AtomicInt return value failure.");
		EATEST_VERIFY_MSG(i64 == 15, "AtomicInt return value failure.");

		// ValueType operator--(int)
		// post-increment operator--
		x64 = i64--;
		EATEST_VERIFY_MSG(x64 == 15, "AtomicInt return value failure.");
		EATEST_VERIFY_MSG(i64 == 14, "AtomicInt return value failure.");
	}

	{ // Basic single-threaded AtomicPointer test.
		AtomicPointer p(NULL);
		AtomicPointer::PointerValueType pTemp;

		EATEST_VERIFY_MSG(p.GetValue() == NULL, "AtomicPointer failure.");

		++p;
		p++;
		--p;
		p--;
		p += 7;
		p -= 3;
		EATEST_VERIFY_MSG(p == (void*)4, "AtomicPointer failure.");

		p = (void*)2;
		pTemp = p.GetValue();
		EATEST_VERIFY_MSG((uintptr_t)pTemp == 2, "AtomicPointer failure.");

		p.Increment();
		p.Decrement();
		p.Add(5);
		p.Add(-2);
		EATEST_VERIFY_MSG(p == (void*)5, "AtomicPointer failure.");

		p.SetValue((void*)6);
		EATEST_VERIFY_MSG(p == (void*)6, "AtomicPointer failure.");

		bool bWasEqualTo10000 = p.SetValueConditional((void*)3, (void*)10000);
		EATEST_VERIFY_MSG(!bWasEqualTo10000, "AtomicPointer failure.");

		bool bWasEqualTo6 = p.SetValueConditional((void*)3, (void*)6);
		EATEST_VERIFY_MSG(bWasEqualTo6, "AtomicPointer failure.");
	}

	{
		AtomicInt32 gA, gB;

		gA = gB = 0;
		++gA;
		++gB;
		gA = gB = 0;

		EATEST_VERIFY_MSG((gA == 0) && (gB == 0), "AtomicInt32 operator= failure.");
	}

	#if EA_THREADS_AVAILABLE

		{  // Multithreaded test 1
			AWorkData32 workData32;

			const int kThreadCount(kMaxConcurrentThreadCount - 1);
			Thread thread[kThreadCount];
			Thread::Status status;

			for(int i(0); i < kThreadCount; i++)
				thread[i].Begin(Atomic32TestThreadFunction1, &workData32);

			EA::UnitTest::ThreadSleepRandom(gTestLengthSeconds*1000, gTestLengthSeconds*1000);

			workData32.mbShouldQuit = true;

			for(int i(0); i < kThreadCount; i++)
			{
				status = thread[i].WaitForEnd(GetThreadTime() + 30000);
				EATEST_VERIFY_MSG(status != EA::Thread::Thread::kStatusRunning, "Atomic/Thread failure.");
			}

			// In the end, the sum must be zero.
			EATEST_VERIFY_MSG(workData32.mnAtomicInteger1 == 0, "Atomic/Thread failure.");
			EATEST_VERIFY_MSG(workData32.mnAtomicInteger2 == 0, "Atomic/Thread failure.");

			nErrorCount += (int)workData32.mnErrorCount;
		}

		{  // Multithreaded test 2
			AWorkData32    workData32;
			const int      kThreadCount(kMaxConcurrentThreadCount - 1);
			Thread         thread[kThreadCount];
			Thread::Status status;

			for(int i(0); i < kThreadCount; i++)
				thread[i].Begin(Atomic32TestThreadFunction2, &workData32);

			EA::UnitTest::ThreadSleepRandom(gTestLengthSeconds*1000, gTestLengthSeconds*1000);

			workData32.mbShouldQuit = true;

			for(int i(0); i < kThreadCount; i++)
			{
				status = thread[i].WaitForEnd(GetThreadTime() + 30000);
				EATEST_VERIFY_MSG(status != EA::Thread::Thread::kStatusRunning, "Atomic/Thread failure.");
			}

			nErrorCount += (int)workData32.mnErrorCount;
		}
	

		{  // Multithreaded test 1
			AWorkData64 workData64;

			const int kThreadCount(kMaxConcurrentThreadCount - 1);
			Thread thread[kThreadCount];
			Thread::Status status;

			for(int i(0); i < kThreadCount; i++)
				thread[i].Begin(Atomic64TestThreadFunction1, &workData64);

			EA::UnitTest::ThreadSleepRandom(gTestLengthSeconds*1000, gTestLengthSeconds*1000);

			workData64.mbShouldQuit = true;

			for(int i(0); i < kThreadCount; i++)
			{
				status = thread[i].WaitForEnd(GetThreadTime() + 30000);
				EATEST_VERIFY_MSG(status != EA::Thread::Thread::kStatusRunning, "Atomic/Thread failure.");
			}

			// In the end, the sum must be zero.
			EATEST_VERIFY_MSG(workData64.mnAtomicInteger1 == 0, "Atomic/Thread failure.");
			EATEST_VERIFY_MSG(workData64.mnAtomicInteger2 == 0, "Atomic/Thread failure.");

			nErrorCount += (int)workData64.mnErrorCount;
		}

		{  // Multithreaded test 2
			AWorkData64    workData64;
			const int      kThreadCount(kMaxConcurrentThreadCount - 1);
			Thread         thread[kThreadCount];
			Thread::Status status;

			for(int i(0); i < kThreadCount; i++)
				thread[i].Begin(Atomic64TestThreadFunction2, &workData64);

			EA::UnitTest::ThreadSleepRandom(gTestLengthSeconds*1000, gTestLengthSeconds*1000);

			workData64.mbShouldQuit = true;

			for(int i(0); i < kThreadCount; i++)
			{
				status = thread[i].WaitForEnd(GetThreadTime() + 30000);
				EATEST_VERIFY_MSG(status != EA::Thread::Thread::kStatusRunning, "Atomic/Thread failure.");
			}

			nErrorCount += (int)workData64.mnErrorCount;
		}

		{  // Multithreaded test 3
			AWorkData64    workData64;
			const int      kThreadCount(kMaxConcurrentThreadCount - 1);
			Thread         thread[kThreadCount];
			Thread::Status status;

			for(int i(0); i < kThreadCount; i++)
				thread[i].Begin(Atomic64TestThreadFunction3, &workData64);

			EA::UnitTest::ThreadSleepRandom(gTestLengthSeconds*1000, gTestLengthSeconds*1000);

			workData64.mbShouldQuit = true;

			for(int i(0); i < kThreadCount; i++)
			{
				status = thread[i].WaitForEnd(GetThreadTime() + 30000);
				EATEST_VERIFY_MSG(status != EA::Thread::Thread::kStatusRunning, "Atomic/Thread failure.");
			}

			nErrorCount += (int)workData64.mnErrorCount;
		}

		#endif

		{
			nErrorCount += TestAtomicIntT<short>();
			nErrorCount += TestAtomicIntT<unsigned short>();
			nErrorCount += TestAtomicIntT<int>();
			nErrorCount += TestAtomicIntT<unsigned int>();
			nErrorCount += TestAtomicIntT<long>();
			nErrorCount += TestAtomicIntT<unsigned long>();
			nErrorCount += TestAtomicIntT<intptr_t>();
			nErrorCount += TestAtomicIntT<uintptr_t>();
			nErrorCount += TestAtomicIntT<size_t>();
			nErrorCount += TestAtomicIntT<int16_t>();
			nErrorCount += TestAtomicIntT<uint16_t>();
			nErrorCount += TestAtomicIntT<int32_t>();
			nErrorCount += TestAtomicIntT<uint32_t>();
			nErrorCount += TestAtomicIntT<char32_t>();
			nErrorCount += TestAtomicIntT<long long>();
			nErrorCount += TestAtomicIntT<unsigned long long>();
			nErrorCount += TestAtomicIntT<int64_t>();
			nErrorCount += TestAtomicIntT<uint64_t>();
		}

		// Non-Member Atomics Tests 
		{
			nErrorCount += TestNonMemberAtomics<short>();
			nErrorCount += TestNonMemberAtomics<unsigned short>();
			nErrorCount += TestNonMemberAtomics<int>();
			nErrorCount += TestNonMemberAtomics<unsigned int>();
			nErrorCount += TestNonMemberAtomics<long>();
			nErrorCount += TestNonMemberAtomics<unsigned long>();
			nErrorCount += TestNonMemberAtomics<intptr_t>();
			nErrorCount += TestNonMemberAtomics<uintptr_t>();
			nErrorCount += TestNonMemberAtomics<size_t>();
			nErrorCount += TestNonMemberAtomics<int16_t>();
			nErrorCount += TestNonMemberAtomics<uint16_t>();
			nErrorCount += TestNonMemberAtomics<int32_t>();
			nErrorCount += TestNonMemberAtomics<uint32_t>();
			nErrorCount += TestNonMemberAtomics<char32_t>();
			nErrorCount += TestNonMemberAtomics<long long>();
			nErrorCount += TestNonMemberAtomics<unsigned long long>();
			nErrorCount += TestNonMemberAtomics<int64_t>();
			nErrorCount += TestNonMemberAtomics<uint64_t>();
		}


	return nErrorCount;
}












