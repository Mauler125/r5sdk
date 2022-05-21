#pragma once

#include <cstdint>
#include "StringBase.h"

namespace Hashing
{
	// A hashing algo that implements CRC32.
	class CRC32
	{
	public:

		// Computes the hash code of the integral value using CRC32 algo.
		template<class Tinput>
		static uint32_t HashValue(Tinput Input, uint32_t Seed = 0)
		{
			return ComputeHash((uint8_t*)&Input, 0, sizeof(Tinput), Seed);
		}

		// Computes the hash code of the input string using CRC32 algo.
		static uint32_t HashString(const string& Input, uint32_t Seed = 0);

		// Computes the hash code using the CRC32 algo.
		static uint32_t ComputeHash(uint8_t* Input, uint64_t InputOffset, uint64_t InputLength, uint32_t Seed = 0);
	};
}