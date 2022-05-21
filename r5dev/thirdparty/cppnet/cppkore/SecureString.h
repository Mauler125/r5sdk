#pragma once

#include "StringBase.h"

namespace Data
{
	namespace _Internal
	{
		template<size_t Index>
		struct Encryptor
		{
			static constexpr void Encrypt(char* Destination, const char* Source, size_t Size)
			{
				Destination[Index] = (char)(Source[Index] ^ Size);
				Encryptor<Index - 1>::Encrypt(Destination, Source, Size + 1);
			}
		};

		template<>
		struct Encryptor<0>
		{
			static constexpr void Encrypt(char* Destination, const char* Source, size_t Size)
			{
				Destination[0] = (char)(Source[0] ^ Size);
			}
		};
	}

	template<size_t Size>
	class SecureString
	{
	public:
		// Initializes a string that is obfuscated during compiletime
		constexpr SecureString(const char(&Str)[Size])
		{
			_Internal::Encryptor<Size - 1>::Encrypt(_Buffer, Str, Size);
		}

		// Returns a decoded version of the encoded string
		string GetDecoded() const
		{
			auto Result = string(_Buffer, (uint32_t)Size);
			auto Key = Size * 2;
			
			for (auto& Ch : Result)
				Ch ^= --Key;

			return Result;
		}

		constexpr operator string(void) const
		{
			auto Result = string(_Buffer, (uint32_t)Size);
			auto Key = Size * 2;

			for (auto& Ch : Result)
				Ch ^= --Key;

			return Result;
		}

	private:
		mutable char _Buffer[Size]{};
	};
}

template<size_t Size>
// Creates a new compile-time encrypted string
static constexpr auto sstring(const char(&String)[Size])
{
	return Data::SecureString(String);
}