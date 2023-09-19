#include "stdafx.h"
#include "Pattern.h"

namespace Data
{
	int64_t Pattern::Search(uint8_t* Buffer, uint64_t Offset, uint64_t Size)
	{
		bool UseSSE = false;
		int CPUIdentifier[4], DataSize = (int)this->_PatternLength;

		// SSE 4.2 support check
		__cpuid(CPUIdentifier, 0);
		if (DataSize <= 16)
		{
			if (CPUIdentifier[0] >= 1)
			{
				__cpuidex(CPUIdentifier, 1, 0);
				UseSSE = ((CPUIdentifier[2] & (1 << 20)) > 0);
			}
		}

		if (!UseSSE)
		{
			auto bFirst = std::memchr(Buffer, this->_Data[0], Size);
			if (bFirst == nullptr)
				return -1;

			while (bFirst)
			{
				bool MatchFound = true;

				for (size_t c = 0; c < DataSize; c++)
				{
					if (this->_Mask[c] == '?')
						continue;

					if ((uint8_t)this->_Data[c] != ((uint8_t*)bFirst)[c])
					{
						MatchFound = false;
						break;
					}
				}

				if (MatchFound)
				{
					return (int64_t)((uint8_t*)bFirst - Buffer);
				}

				bFirst = std::memchr((uint8_t*)bFirst + 1, this->_Data[0], Size - ((uint8_t*)bFirst - Buffer));
			}
		}
		else
		{
			__declspec(align(16)) char DesiredMask[16] = { 0 };

			for (size_t i = 0; i < DataSize; i++)
				DesiredMask[i / 8] |= ((this->_Mask[i] == '?') ? 0 : 1) << (i % 8);

			__m128i Mask = _mm_load_si128(reinterpret_cast<const __m128i*>(DesiredMask));
			__m128i Comparand = _mm_loadu_si128(reinterpret_cast<const __m128i*>(this->_Data));

			for (uint64_t i = Offset; i <= (Offset + Size); i++)
			{
				__m128i Value = _mm_loadu_si128(reinterpret_cast<const __m128i*>(Buffer + i));
				__m128i Result = _mm_cmpestrm(Value, 16, Comparand, DataSize, _SIDD_CMP_EQUAL_EACH);

				__m128i Matches = _mm_and_si128(Mask, Result);
				__m128i Equivalence = _mm_xor_si128(Mask, Matches);

				if (_mm_test_all_zeros(Equivalence, Equivalence))
					return (int64_t)(i - Offset);
			}
		}

		// We failed to find the pattern...
		return -1;
	}

	List<int64_t> Pattern::SearchAll(uint8_t* Buffer, uint64_t Offset, uint64_t Size)
	{
		auto ResultList = List<int64_t>();

		bool UseSSE = false;
		int CPUIdentifier[4], DataSize = (int)this->_PatternLength;

		// SSE 4.2 support check
		__cpuid(CPUIdentifier, 0);
		if (DataSize <= 16)
		{
			if (CPUIdentifier[0] >= 1)
			{
				__cpuidex(CPUIdentifier, 1, 0);
				UseSSE = ((CPUIdentifier[2] & (1 << 20)) > 0);
			}
		}

		if (!UseSSE)
		{
			auto bFirst = std::memchr(Buffer, this->_Data[0], Size);
			if (bFirst == nullptr)
				return -1;

			while (bFirst)
			{
				bool MatchFound = true;

				for (size_t c = 0; c < DataSize; c++)
				{
					if (this->_Mask[c] == '?')
						continue;

					if ((uint8_t)this->_Data[c] != ((uint8_t*)bFirst)[c])
					{
						MatchFound = false;
						break;
					}
				}

				if (MatchFound)
				{
					ResultList.EmplaceBack(((uint8_t*)bFirst - Buffer));
				}

				bFirst = std::memchr((uint8_t*)bFirst + 1, this->_Data[0], Size - ((uint8_t*)bFirst - Buffer));
			}
		}
		else
		{
			__declspec(align(16)) char DesiredMask[16] = { 0 };

			for (size_t i = 0; i < DataSize; i++)
				DesiredMask[i / 8] |= ((this->_Mask[i] == '?') ? 0 : 1) << (i % 8);

			__m128i Mask = _mm_load_si128(reinterpret_cast<const __m128i*>(DesiredMask));
			__m128i Comparand = _mm_loadu_si128(reinterpret_cast<const __m128i*>(this->_Data));

			for (uint64_t i = Offset; i <= (Offset + Size); i++)
			{
				__m128i Value = _mm_loadu_si128(reinterpret_cast<const __m128i*>(Buffer + i));
				__m128i Result = _mm_cmpestrm(Value, 16, Comparand, DataSize, _SIDD_CMP_EQUAL_EACH);

				__m128i Matches = _mm_and_si128(Mask, Result);
				__m128i Equivalence = _mm_xor_si128(Mask, Matches);

				if (_mm_test_all_zeros(Equivalence, Equivalence))
					ResultList.EmplaceBack((int64_t)(i - Offset));
			}
		}

		return ResultList;
	}
}