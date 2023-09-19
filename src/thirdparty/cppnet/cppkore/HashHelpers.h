#pragma once

#include <cstdint>
#include <cmath>
#include <algorithm>

// Provides data for various hashtable implementations
class HashHelpers
{
private:

	// Pre-calculated table of primes
	constexpr static const int Primes[] =
	{
			3, 7, 11, 17, 23, 29, 37, 47, 59, 71, 89, 107, 131, 163, 197, 239, 293, 353, 431, 521, 631, 761, 919,
			1103, 1327, 1597, 1931, 2333, 2801, 3371, 4049, 4861, 5839, 7013, 8419, 10103, 12143, 14591,
			17519, 21023, 25229, 30293, 36353, 43627, 52361, 62851, 75431, 90523, 108631, 130363, 156437,
			187751, 225307, 270371, 324449, 389357, 467237, 560689, 672827, 807403, 968897, 1162687, 1395263,
			1674319, 2009191, 2411033, 2893249, 3471899, 4166287, 4999559, 5999471, 7199369
	};

	// A good hash prime value
	constexpr static int HashPrime = 101;
	// This is the maximum prime smaller than array length
	constexpr static int MaxPrimeArrayLength = 0x7FEFFFFD;

public:

	// Threshhold used when calculating hashes
	constexpr static int HashCollisionThreshhold = 100;

	// Calculates whether or not the candidate is a prime number
	constexpr static bool IsPrime(uint32_t Candidate)
	{
		if ((Candidate & 1) != 0)
		{
			auto Limit = (uint32_t)std::sqrt(Candidate);
			for (uint32_t Divisor = 3; Divisor <= Limit; Divisor += 2)
			{
				if ((Candidate % Divisor) == 0)
					return false;
			}

			return true;
		}

		return (Candidate == 2);
	}

	// Gets the nearest prime number
	constexpr static uint32_t GetPrime(uint32_t Min)
	{
		for (const auto& Prime : Primes)
		{
			if ((uint32_t)Prime >= Min)
				return Prime;
		}

		for (uint32_t i = (Min | 1); i < INT32_MAX; i += 2)
		{
			if (IsPrime(i) && ((i - 1) % HashPrime != 0))
				return i;
		}

		return Min;
	}

	// Returns the new prime size
	constexpr static uint32_t ExpandPrime(uint32_t OldSize)
	{
		auto NewSize = 2 * OldSize;

		if (NewSize > MaxPrimeArrayLength && MaxPrimeArrayLength > OldSize)
			return MaxPrimeArrayLength;

		return GetPrime(NewSize);
	}
};