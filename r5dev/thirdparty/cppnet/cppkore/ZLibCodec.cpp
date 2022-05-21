#include "stdafx.h"
#include "ZLibCodec.h"

#include "..\cppkore_incl\ZLib\miniz.h"

#if _DEBUG
#pragma comment(lib, "..\\cppkore_libs\\ZLib\\cppkorezlibx64d.lib")
#else
#pragma comment(lib, "..\\cppkore_libs\\ZLib\\cppkorezlibx64r.lib")
#endif

namespace Compression
{
	uint64_t ZLibCodec::Compress(uint8_t* Input, uint64_t InputOffset, uint64_t InputLength, uint8_t* Output, uint64_t OutputOffset, uint64_t OutputLength)
	{
		mz_ulong ResultSize = (mz_ulong)OutputLength;
		
		if (compress(Output + OutputOffset, &ResultSize, Input + InputOffset, (mz_ulong)InputLength) == MZ_OK)
			return ResultSize;

		return 0;
	}

	std::unique_ptr<uint8_t[]> ZLibCodec::Compress(uint8_t* Input, uint64_t InputOffset, uint64_t InputLength, uint64_t& OutputLength)
	{
		auto ResultBounds = compressBound((mz_ulong)InputLength);
		auto Result = std::make_unique<uint8_t[]>(ResultBounds);

		OutputLength = 0;

		if (compress(Result.get(), &ResultBounds, Input + InputOffset, (mz_ulong)InputLength) != MZ_OK)
			return nullptr;

		OutputLength = (uint64_t)ResultBounds;

		return Result;
	}

	uint64_t ZLibCodec::Decompress(uint8_t* Input, uint64_t InputOffset, uint64_t InputLength, uint8_t* Output, uint64_t OutputOffset, uint64_t OutputLength)
	{
		mz_ulong ResultSize = (mz_ulong)OutputLength;

		if (uncompress(Output + OutputOffset, &ResultSize, Input + InputOffset, (mz_ulong)InputLength) == MZ_OK)
			return ResultSize;

		return 0;
	}

	std::unique_ptr<uint8_t[]> ZLibCodec::Decompress(uint8_t* Input, uint64_t InputOffset, uint64_t InputLength, uint64_t KnownOutputLength)
	{
		if (InputLength == 0)
			return nullptr;

		mz_ulong ResultLength = (mz_ulong)KnownOutputLength;
		auto Result = std::make_unique<uint8_t[]>(KnownOutputLength);

		if (uncompress(Result.get(), &ResultLength, Input + InputOffset, (mz_ulong)InputLength) != MZ_OK)
			return nullptr;

		return Result;
	}
}
