#ifndef TIER0_SIGMAKER_H
#define TIER0_SIGMAKER_H

// This code allows you to turn a string literal into a byte pattern at compile
// time. The string pattern "40 53 ?? 83 ? 20 48" is 19 bytes, each nibble
// counts as a byte as its a string literal. The plan is to combine them, and
// remove all the spaces to turn it into a byte array which will take up
// 7 bytes total in the resulting binary. The signature and mask are always
// equal in length, and neither of the 2 buffers are null terminated. The size
// can be retrieved with GetData.size() or GetMask().size().
namespace SigCompileTime
{
	// NOTE: this is V_nibble from src/tier1/strtools.cpp, but made constexpr
	// so we can turn our patterns into actual byte arrays at compile time.
	constexpr unsigned char V_nibble_compiletime(const char c)
	{
		if ((c >= '0') &&
			(c <= '9'))
		{
			return (unsigned char)(c - '0');
		}

		if ((c >= 'A') &&
			(c <= 'F'))
		{
			return (unsigned char)(c - 'A' + 0x0a);
		}

		if ((c >= 'a') &&
			(c <= 'f'))
		{
			return (unsigned char)(c - 'a' + 0x0a);
		}

		return '0';
	}

	constexpr static bool IsWhitespace(const char c)
	{
		return c == ' ';
	}

	constexpr static bool IsDoubleWildCard(const char cur, const char prev)
	{
		return (cur == '?' && prev == '?');
	}

	template <size_t N>
	struct CSigMaker
	{
	private:
		std::array<unsigned char, N> m_data{};
		std::array<char, N> m_mask{};

	public:
		template <typename StringHolder>
		constexpr CSigMaker(StringHolder str)
		{
			constexpr std::string_view view = str();
			size_t index = 0;
			char prev = '\0';

			for (size_t i = 0; i < view.length();)
			{
				const char cur = view[i];

				if (!IsWhitespace(cur) && !IsDoubleWildCard(cur, prev))
				{
					if (cur != '?')
					{
						m_data[index] = (V_nibble_compiletime(cur) << 4) | V_nibble_compiletime(view[i + 1]);
						m_mask[index] = 'x';

						i++; // Skip the next nibble.
					}
					else
					{
						// Wild-card token = null byte.
						m_data[index] = '\x00';
						m_mask[index] = '?';
					}

					index++;
				}

				prev = cur;
				i++;
			}
		}

		constexpr const std::array<unsigned char, N>& GetData() const
		{
			return m_data;
		}
		constexpr const std::array<char, N>& GetMask() const
		{
			return m_mask;
		}
	};

	// Calculate the buffer size in advance, since it will be smaller than our
	// string literal as a nibble in the string is a byte in the binary.
	template <typename StringHolder>
	constexpr size_t CountTotalBufSize(StringHolder str)
	{
		constexpr std::string_view view = str();

		size_t count = 0;
		char prev = '\0';

		for (size_t i = 0; i < view.length();)
		{
			const char cur = view[i];

			if (!IsWhitespace(cur) && !IsDoubleWildCard(cur, prev))
			{
				if (cur != '?')
				{
					// Skip the next nibble because
					// each string nibble is 1 byte.
					i++;
				}

				count++;
			}

			prev = cur;
			i++;
		}

		return count;
	}

	template <typename StringHolder>
	constexpr auto CreateSignature(StringHolder str)
	{
		constexpr size_t newSize = CountTotalBufSize(str);
		return CSigMaker<newSize>(str);
	}
}

// Note(amos): see https://stackoverflow.com/a/58446127                           - 
//             and https://blog.therocode.net/2018/09/compile-time-string-parsing - 
// In order to use the string literal as constexpr, we wrap it in a constexpr lambda
// expression. This is valid C++17 code, however the usage of the deduced value in
// SigCompileTime::CreateSignature(...) breaks compatibility with MSVC 15.6.7. The
// compilers from around that time had problems with constexpr so we probably have
// to drop support for the old VS2017 compilers. The optimization produced by this
// static generation of function patterns however, makes it worth the move.
#define SigMaker_CaptureStrSignature( pattern ) []() { return pattern; }
#define SigMaker_CreateCodeSignature( pattern ) SigCompileTime::CreateSignature( pattern )

#endif // TIER0_SIGMAKER_H
