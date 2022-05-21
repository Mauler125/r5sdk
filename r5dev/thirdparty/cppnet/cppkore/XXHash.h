#pragma once

#include <cstdint>
#include "StringBase.h"

namespace Hashing
{
	// This enumeration represents the various supported xxhash versions
	enum class XXHashVersion
	{
		// A 32bit hash value is returned.
		XX32,
		// A 64bit hash value is returned.
		XX64
	};

	// A hashing algo that implements XXHash.
	class XXHash
	{
	public:
		
		// Computes the hash code of the integral value using XXHash algo.
		template<class Tinput>
		static uint64_t HashValue(Tinput Input, XXHashVersion Version = XXHashVersion::XX64, uint64_t Seed = 0)
		{
			return ComputeHash((uint8_t*)&Input, 0, sizeof(Tinput), Version, Seed);
		}

		// Computes the hash code of the input string using XXHash algo.
		static uint64_t HashString(const string& Input, XXHashVersion Version = XXHashVersion::XX64, uint64_t Seed = 0);

		// Computes the hash code using the XXHash algo.
		static uint64_t ComputeHash(uint8_t* Input, uint64_t InputOffset, uint64_t InputLength, XXHashVersion Version = XXHashVersion::XX64, uint64_t Seed = 0);
	};
}