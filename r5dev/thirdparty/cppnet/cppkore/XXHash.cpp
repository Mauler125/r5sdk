#include "stdafx.h"
#include "XXHash.h"

#include "..\cppkore_incl\LZ4_XXHash\xxhash.h"

#if _WIN64
#pragma comment(lib, "..\\cppkore_libs\\LZ4_XXHash\\liblz4_static_64.lib")
#else
#pragma comment(lib, "..\\cppkore_libs\\LZ4_XXHash\\liblz4_static_32.lib")
#endif

namespace Hashing
{
	uint64_t XXHash::HashString(const string& Input, XXHashVersion Version, uint64_t Seed)
	{
		return ComputeHash((uint8_t*)(char*)Input, 0, Input.Length(), Version, Seed);
	}

	uint64_t XXHash::ComputeHash(uint8_t* Input, uint64_t InputOffset, uint64_t InputLength, XXHashVersion Version, uint64_t Seed)
	{
		if (Version == XXHashVersion::XX64)
			return XXH64(Input + InputOffset, (size_t)InputLength, (uint64_t)Seed);
		else
			return XXH32(Input + InputOffset, (size_t)InputLength, (uint32_t)Seed);
	}
}
