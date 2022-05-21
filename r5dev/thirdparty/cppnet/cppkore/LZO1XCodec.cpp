#include "stdafx.h"
#include "LZO1XCodec.h"

#include "..\cppkore_incl\LZO1X\minilzo.h"

#if _DEBUG
#pragma comment(lib, "..\\cppkore_libs\\LZO1X\\cppkorelzox64d.lib")
#else
#pragma comment(lib, "..\\cppkore_libs\\LZO1X\\cppkorelzox64r.lib")
#endif

namespace Compression
{
	constexpr uint64_t LZO1X_CompressBound(uint64_t Size)
	{
		return (Size + Size / 16 + 64 + 3);
	}

	uint64_t LZO1XCodec::Compress(uint8_t* Input, uint64_t InputOffset, uint64_t InputLength, uint8_t* Output, uint64_t OutputOffset, uint64_t OutputLength)
	{
		auto WorkingMemory = malloc(LZO1X_1_MEM_COMPRESS);

		lzo_uint ResultSize = (lzo_uint)OutputLength;
		auto Result = lzo1x_1_compress(Input + InputOffset, (lzo_uint)InputLength, Output + OutputOffset, &ResultSize, WorkingMemory);

		if (WorkingMemory != nullptr)
			free(WorkingMemory);

		if (Result == LZO_E_OK)
			return (uint64_t)ResultSize;

		return 0;
	}

	std::unique_ptr<uint8_t[]> LZO1XCodec::Compress(uint8_t* Input, uint64_t InputOffset, uint64_t InputLength, uint64_t& OutputLength)
	{
		auto ResultBounds = LZO1X_CompressBound(InputLength);
		auto Result = std::make_unique<uint8_t[]>(ResultBounds);
		auto WorkingMemory = malloc(LZO1X_1_MEM_COMPRESS);

		OutputLength = 0;

		lzo_uint ResultSize = (lzo_uint)ResultBounds;
		auto ResultCode = lzo1x_1_compress(Input + InputOffset, (lzo_uint)InputLength, Result.get(), &ResultSize, WorkingMemory);

		if (WorkingMemory != nullptr)
			free(WorkingMemory);

		if (ResultCode != LZO_E_OK)
			return nullptr;

		OutputLength = (uint64_t)ResultSize;

		return Result;
	}

	uint64_t LZO1XCodec::Decompress(uint8_t* Input, uint64_t InputOffset, uint64_t InputLength, uint8_t* Output, uint64_t OutputOffset, uint64_t OutputLength)
	{
		lzo_uint ResultSize = (lzo_uint)OutputLength;

		if (lzo1x_decompress_safe(Input + InputOffset, (lzo_uint)InputLength, Output + OutputOffset, &ResultSize, nullptr) == LZO_E_OK)
			return (uint64_t)ResultSize;

		return 0;
	}

	std::unique_ptr<uint8_t[]> LZO1XCodec::Decompress(uint8_t* Input, uint64_t InputOffset, uint64_t InputLength, uint64_t KnownOutputLength)
	{
		auto Result = std::make_unique<uint8_t[]>(KnownOutputLength);
		lzo_uint ResultSize = (lzo_uint)KnownOutputLength;

		if (lzo1x_decompress_safe(Input + InputOffset, (lzo_uint)InputLength, Result.get(), &ResultSize, nullptr) == LZO_E_OK)
			return Result;

		return nullptr;
	}
}
