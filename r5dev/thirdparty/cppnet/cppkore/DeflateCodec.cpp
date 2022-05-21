#include "stdafx.h"
#include "DeflateCodec.h"

#include "..\cppkore_incl\ZLib\miniz.h"

#if _DEBUG
#pragma comment(lib, "..\\cppkore_libs\\ZLib\\cppkorezlibx64d.lib")
#else
#pragma comment(lib, "..\\cppkore_libs\\ZLib\\cppkorezlibx64r.lib")
#endif

namespace Compression
{
	uint64_t DeflateCodec::Compress(uint8_t* Input, uint64_t InputOffset, uint64_t InputLength, uint8_t* Output, uint64_t OutputOffset, uint64_t OutputLength)
	{
		z_stream DeflateStream{};

		if (deflateInit2(&DeflateStream, MZ_DEFAULT_LEVEL, MZ_DEFLATED, -MZ_DEFAULT_WINDOW_BITS, 9, MZ_DEFAULT_STRATEGY) != MZ_OK)
			return 0;

		DeflateStream.avail_in = (uint32_t)InputLength;
		DeflateStream.avail_out = (uint32_t)OutputLength;
		DeflateStream.next_in = (const uint8_t*)(Input + InputOffset);
		DeflateStream.next_out = (uint8_t*)(Output + OutputOffset);

		auto Result = deflate(&DeflateStream, MZ_SYNC_FLUSH);

		deflateEnd(&DeflateStream);

		if (Result == MZ_OK)
			return (uint64_t)DeflateStream.total_out;

		return 0;
	}

	std::unique_ptr<uint8_t[]> DeflateCodec::Compress(uint8_t* Input, uint64_t InputOffset, uint64_t InputLength, uint64_t& OutputLength)
	{
		auto ResultBounds = compressBound((mz_ulong)InputLength);
		auto Result = std::make_unique<uint8_t[]>(ResultBounds);

		OutputLength = 0;

		z_stream DeflateStream{};

		if (deflateInit2(&DeflateStream, MZ_DEFAULT_LEVEL, MZ_DEFLATED, -MZ_DEFAULT_WINDOW_BITS, 9, MZ_DEFAULT_STRATEGY) != MZ_OK)
			return nullptr;

		DeflateStream.avail_in = (uint32_t)InputLength;
		DeflateStream.avail_out = (uint32_t)ResultBounds;
		DeflateStream.next_in = (const uint8_t*)(Input + InputOffset);
		DeflateStream.next_out = (uint8_t*)Result.get();

		auto ResultCode = deflate(&DeflateStream, MZ_SYNC_FLUSH);

		deflateEnd(&DeflateStream);

		if (ResultCode != MZ_OK)
			return nullptr;

		OutputLength = (uint64_t)DeflateStream.total_out;

		return Result;
	}

	uint64_t DeflateCodec::Decompress(uint8_t* Input, uint64_t InputOffset, uint64_t InputLength, uint8_t* Output, uint64_t OutputOffset, uint64_t OutputLength)
	{
		z_stream DeflateStream{};

		if (inflateInit2(&DeflateStream, -MZ_DEFAULT_WINDOW_BITS) != MZ_OK)
			return 0;

		DeflateStream.avail_in = (uint32_t)InputLength;
		DeflateStream.avail_out = (uint32_t)OutputLength;
		DeflateStream.next_in = (const uint8_t*)(Input + InputOffset);
		DeflateStream.next_out = (uint8_t*)(Output + OutputOffset);

		auto Result = inflate(&DeflateStream, MZ_SYNC_FLUSH);

		inflateEnd(&DeflateStream);

		if (Result == MZ_OK || (DeflateStream.total_out == OutputLength))
			return (uint64_t)DeflateStream.total_out;

		return 0;
	}

	std::unique_ptr<uint8_t[]> DeflateCodec::Decompress(uint8_t* Input, uint64_t InputOffset, uint64_t InputLength, uint64_t KnownOutputLength)
	{
		if (InputLength == 0)
			return nullptr;

		auto Result = std::make_unique<uint8_t[]>(KnownOutputLength);

		z_stream DeflateStream{};

		if (inflateInit2(&DeflateStream, -MZ_DEFAULT_WINDOW_BITS) != MZ_OK)
			return 0;

		DeflateStream.avail_in = (uint32_t)InputLength;
		DeflateStream.avail_out = (uint32_t)KnownOutputLength;
		DeflateStream.next_in = (const uint8_t*)(Input + InputOffset);
		DeflateStream.next_out = (uint8_t*)Result.get();

		auto ResultCode = inflate(&DeflateStream, MZ_SYNC_FLUSH);

		inflateEnd(&DeflateStream);

		if (ResultCode == MZ_OK || (DeflateStream.total_out == KnownOutputLength))
			return Result;

		return nullptr;
	}
}
