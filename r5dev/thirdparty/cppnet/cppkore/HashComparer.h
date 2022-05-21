#pragma once

#include <cstdint>
#include "XXHash.h"

template<class TType>
struct HashComparer
{
	constexpr static uint64_t GetHashCode(TType& Value)
	{
		if constexpr (std::is_arithmetic<TType>::value)
			return Hashing::XXHash::HashValue(Value);
		else if constexpr (std::is_enum<TType>::value)
			return Hashing::XXHash::HashValue((uint32_t)Value);
		else if constexpr (std::is_same<TType, string>::value)
			return Hashing::XXHash::HashString(Value);
	}

	constexpr static bool Equals(TType& Lhs, TType& Rhs)
	{
		return (Lhs == Rhs);
	}
};