#include "stdafx.h"
#include "LZ4Codec.h"

#include "..\cppkore_incl\LZ4_XXHash\lz4.h"
#include "..\cppkore_incl\LZ4_XXHash\lz4hc.h"

#if _WIN64
#pragma comment(lib, "..\\cppkore_libs\\LZ4_XXHash\\liblz4_static_64.lib")
#else
#pragma comment(lib, "..\\cppkore_libs\\LZ4_XXHash\\liblz4_static_32.lib")
#endif

namespace Compression
{
	uint64_t LZ4Codec::Compress(uint8_t* Input, uint64_t InputOffset, uint64_t InputLength, uint8_t* Output, uint64_t OutputOffset, uint64_t OutputLength)
	{
		return LZ4_compress_default((const char*)(Input + InputOffset), (char*)(Output + OutputOffset), (int)InputLength, (int)OutputLength);
	}

	std::unique_ptr<uint8_t[]> LZ4Codec::Compress(uint8_t* Input, uint64_t InputOffset, uint64_t InputLength, uint64_t& OutputLength)
	{
		auto ResultBounds = LZ4_compressBound((int)InputLength);
		auto Result = std::make_unique<uint8_t[]>(ResultBounds);

		OutputLength = 0;

		auto Encoded = LZ4_compress_default((const char*)(Input + InputOffset), (char*)Result.get(), (int)InputLength, ResultBounds);
		if (Encoded == 0)
			return nullptr;

		OutputLength = (uint64_t)Encoded;

		return Result;
	}

	uint64_t LZ4Codec::CompressHC(uint8_t* Input, uint64_t InputOffset, uint64_t InputLength, uint8_t* Output, uint64_t OutputOffset, uint64_t OutputLength, LZ4CompressionLevel Level)
	{
		return LZ4_compress_HC((const char*)(Input + InputOffset), (char*)(Output + OutputOffset), (int)InputLength, (int)OutputLength, (uint8_t)Level);
	}

	std::unique_ptr<uint8_t[]> LZ4Codec::CompressHC(uint8_t* Input, uint64_t InputOffset, uint64_t InputLength, uint64_t& OutputLength, LZ4CompressionLevel Level)
	{
		auto ResultBounds = LZ4_compressBound((int)InputLength);
		auto Result = std::make_unique<uint8_t[]>(ResultBounds);

		OutputLength = 0;

		auto Encoded = LZ4_compress_HC((const char*)(Input + InputOffset), (char*)Result.get(), (int)InputLength, ResultBounds, (uint8_t)Level);
		if (Encoded == 0)
			return nullptr;

		OutputLength = (uint64_t)Encoded;

		return Result;
	}

	uint64_t LZ4Codec::Decompress(uint8_t* Input, uint64_t InputOffset, uint64_t InputLength, uint8_t* Output, uint64_t OutputOffset, uint64_t OutputLength)
	{
		return LZ4_decompress_safe((const char*)(Input + InputOffset), (char*)(Output + OutputOffset), (int)InputLength, (int)OutputLength);
	}

	std::unique_ptr<uint8_t[]> LZ4Codec::Decompress(uint8_t* Input, uint64_t InputOffset, uint64_t InputLength, uint64_t KnownOutputLength)
	{
		if (InputLength == 0)
			return nullptr;

		auto Result = std::make_unique<uint8_t[]>(KnownOutputLength);

		auto ResultLength = LZ4_decompress_safe((const char*)(Input + InputOffset), (char*)Result.get(), (int)InputLength, (int)KnownOutputLength);
		if (ResultLength <= 0)
			return nullptr;

		return Result;
	}
}
