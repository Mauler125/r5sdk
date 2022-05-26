#pragma once

#include <cstdint>
#include <limits>

//
// Contains stdlib extensions that aren't provided cross platform
//

namespace std
{
	template<size_t index, typename T, typename... Ts>
	inline constexpr typename std::enable_if<index == 0, T>::type get_param(T&& t, Ts&&... ts)
	{
		return t;
	}

	template<size_t index, typename T, typename... Ts>
	inline constexpr typename std::enable_if<(index > 0) && index <= sizeof...(Ts), typename std::tuple_element<index, std::tuple<T, Ts...>>::type>::type get_param(T&& t, Ts&&... ts)
	{
		return get_param<index - 1>(std::forward<Ts>(ts)...);
	}

	inline void* memrchr(_In_reads_bytes_opt_(_N) void* _Pv, _In_ int _C, _In_ size_t _N)
	{
		const unsigned char *char_ptr;
		const unsigned long int *longword_ptr;
		unsigned long int longword, magic_bits, charmask;
		unsigned char c;
		int i;

		c = (unsigned char)_C;

		for (char_ptr = (const unsigned char*)_Pv + _N;
			_N > 0 && (size_t)char_ptr % sizeof longword != 0;
			--_N)
			if (*--char_ptr == c)
				return (void*)char_ptr;

		longword_ptr = (const unsigned long int*)char_ptr;

		magic_bits = 0xfefefefe;
		charmask = c | (c << 8);
		charmask |= charmask << 16;
#if 0xffffffffU < ULONG_MAX
		magic_bits |= magic_bits << 32;
		charmask |= charmask << 32;
		if (8 < sizeof longword)
			for (i = 64; i < sizeof longword * 8; i *= 2)
			{
				magic_bits |= magic_bits << i;
				charmask |= charmask << i;
			}
#endif
		magic_bits = (ULONG_MAX >> 1) & (magic_bits | 1);

		while (_N >= sizeof longword)
		{
			longword = *--longword_ptr ^ charmask;

			if ((((longword + magic_bits) ^ ~longword) & ~magic_bits) != 0)
			{
				const unsigned char *cp = (const unsigned char*)longword_ptr;

				if (8 < sizeof longword)
					for (i = sizeof longword - 1; 8 <= i; i--)
						if (cp[i] == c)
							return (void *)&cp[i];
				if (7 < sizeof longword && cp[7] == c)
					return (void *)&cp[7];
				if (6 < sizeof longword && cp[6] == c)
					return (void *)&cp[6];
				if (5 < sizeof longword && cp[5] == c)
					return (void *)&cp[5];
				if (4 < sizeof longword && cp[4] == c)
					return (void *)&cp[4];
				if (cp[3] == c)
					return (void *)&cp[3];
				if (cp[2] == c)
					return (void *)&cp[2];
				if (cp[1] == c)
					return (void *)&cp[1];
				if (cp[0] == c)
					return (void *)cp;
			}

			_N -= sizeof longword;
		}

		char_ptr = (const unsigned char*)longword_ptr;

		while (_N-- > 0)
		{
			if (*--char_ptr == c)
				return (void*)char_ptr;
		}

		return 0;
	}

	inline const wchar_t* wmemrchr(_In_reads_opt_(_N) const wchar_t* _Pv, _In_ wchar_t _C, _In_ size_t _N)
	{
		while (_N--)
			if (_Pv[_N] == _C)
				return _Pv + _N;

		return 0;
	}
}