#pragma once

#include "ListBase.h"
#include "StringBase.h"
#include "SecureString.h"

namespace Data
{
	// Pattern handles signature matching in data buffers
	class Pattern
	{
	public:
		constexpr Pattern(const string& Signature)
			: _PatternLength(0)
		{
			// Process the signature into a data / mask combo for later scanning
			uint8_t DigitBuffer = 0;
			bool LowerBound = false, LastWasUnknown = false;

			for (auto& Ch : Signature)
			{
				if (Ch == ' ')
				{
					LastWasUnknown = false;
					continue;
				}
				else if (Ch == '?')
				{
					if (LastWasUnknown)
						LastWasUnknown = false;
					else
					{
						_Data[_PatternLength] = '\x00';
						_Mask[_PatternLength++] = '?';
						LastWasUnknown = true;
					}
				}
				else if ((Ch >= '0' && Ch <= '9') || (Ch >= 'A' && Ch <= 'F') || (Ch >= 'a' && Ch <= 'f'))
				{
					int ThisDigit = (Ch >= 'A') ? (Ch >= 'a') ? (Ch - 'a' + 10) : (Ch - 'A' + 10) : (Ch - '0');

					if (!LowerBound)
					{
						DigitBuffer = (ThisDigit << 4);
						LowerBound = true;
					}
					else
					{
						DigitBuffer |= ThisDigit;
						LowerBound = false;

						_Data[_PatternLength] = DigitBuffer;
						_Mask[_PatternLength++] = 'x';
					}
				}
			}
		}

		// Gets the size of the current data
		constexpr size_t DataSize() const
		{
			return _PatternLength;
		}

		// Gets the size of the current mask
		constexpr size_t MaskSize() const
		{
			return _PatternLength;
		}

		// Search for the pattern in the given buffer
		int64_t Search(uint8_t* Buffer, uint64_t Offset, uint64_t Size);
		// Search for all occurences of the pattern in the given buffer
		List<int64_t> SearchAll(uint8_t* Buffer, uint64_t Offset, uint64_t Size);

	private:
		char _Data[64]{};
		char _Mask[64]{};
		uint16_t _PatternLength;
	};
}