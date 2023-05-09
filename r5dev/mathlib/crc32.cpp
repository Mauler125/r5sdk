#include "mathlib/crc32.h"

// Karl Malbrain's compact CRC-32, with pre and post conditioning. 
// See "A compact CCITT crc16 and crc32 C implementation that balances processor cache usage against speed": 
// http://www.geocities.com/malbrain/
uint32_t crc32::update(uint32_t crc, const uint8_t* ptr, size_t buf_len)
{
	if (!ptr)
	{
		return NULL;
	}

	crc = ~crc;
	while (buf_len--)
	{
		uint8_t b = *ptr++;
		crc = (crc >> 4) ^ s_crc32[(crc & 0xF) ^ (b & 0xF)];
		crc = (crc >> 4) ^ s_crc32[(crc & 0xF) ^ (b >> 4)];
	}
	return ~crc;
}
