#pragma once

#include <memory>
#include <cstdint>

namespace Compression
{
	// A compression codec that implements the LZHAM algo.
	class LZHAMCodec
	{
	public:

		// Compress the input buffer using the LZHAM codec
		static uint64_t Compress(uint8_t* Input, uint64_t InputOffset, uint64_t InputLength, uint8_t* Output, uint64_t OutputOffset, uint64_t OutputLength, uint32_t DictSize = 28);
		// Compress the input buffer using the LZHAM codec
		static std::unique_ptr<uint8_t[]> Compress(uint8_t* Input, uint64_t InputOffset, uint64_t InputLength, uint64_t& OutputLength, uint32_t DictSize = 28);

		// Decompress the input buffer using the LZHAM codec
		static uint64_t Decompress(uint8_t* Input, uint64_t InputOffset, uint64_t InputLength, uint8_t* Output, uint64_t OutputOffset, uint64_t OutputLength, uint32_t DictSize = 28);
		// Decompress the input buffer using the LZHAM codec (With known output length)
		static std::unique_ptr<uint8_t[]> Decompress(uint8_t* Input, uint64_t InputOffset, uint64_t InputLength, uint64_t KnownOutputLength, uint32_t DictSize = 28);
	};
}