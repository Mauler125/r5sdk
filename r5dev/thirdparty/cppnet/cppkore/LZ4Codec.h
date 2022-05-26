#pragma once

#include <memory>
#include <cstdint>

namespace Compression
{
	// This enumeration represents the various supported compression levels
	enum class LZ4CompressionLevel : uint8_t
	{
		Min = 3,
		Default = 9,
		OptMin = 10,
		Max = 12
	};

	// A compression codec that implements the LZ4 algo.
	class LZ4Codec
	{
	public:

		// Compress the input buffer using the LZ4 codec
		static uint64_t Compress(uint8_t* Input, uint64_t InputOffset, uint64_t InputLength, uint8_t* Output, uint64_t OutputOffset, uint64_t OutputLength);
		// Compress the input buffer using the LZ4 codec
		static std::unique_ptr<uint8_t[]> Compress(uint8_t* Input, uint64_t InputOffset, uint64_t InputLength, uint64_t& OutputLength);

		// Compress the input buffer using the LZ4 codec (High Compression)
		static uint64_t CompressHC(uint8_t* Input, uint64_t InputOffset, uint64_t InputLength, uint8_t* Output, uint64_t OutputOffset, uint64_t OutputLength, LZ4CompressionLevel Level = LZ4CompressionLevel::Max);
		// Compress the input buffer using the LZ4 codec (High Compression)
		static std::unique_ptr<uint8_t[]> CompressHC(uint8_t* Input, uint64_t InputOffset, uint64_t InputLength, uint64_t& OutputLength, LZ4CompressionLevel Level = LZ4CompressionLevel::Max);

		// Decompress the input buffer using the LZ4 codec
		static uint64_t Decompress(uint8_t* Input, uint64_t InputOffset, uint64_t InputLength, uint8_t* Output, uint64_t OutputOffset, uint64_t OutputLength);
		// Decompress the input buffer using the LZ4 codec (With known output length)
		static std::unique_ptr<uint8_t[]> Decompress(uint8_t* Input, uint64_t InputOffset, uint64_t InputLength, uint64_t KnownOutputLength);
	};
}