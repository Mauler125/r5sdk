#include "stdafx.h"
#include "LZHAMCodec.h"

#define LZHAM_STATIC_LIB 1

#include "..\cppkore_incl\LZHAM\lzham.h"

#if _WIN64
#if _DEBUG
#pragma comment(lib, "..\\cppkore_libs\\LZHAM\\lzhamlib_x64D.lib")
#pragma comment(lib, "..\\cppkore_libs\\LZHAM\\lzhamcomp_x64D.lib")
#pragma comment(lib, "..\\cppkore_libs\\LZHAM\\lzhamdecomp_x64D.lib")
#else
#pragma comment(lib, "..\\cppkore_libs\\LZHAM\\lzhamlib_x64.lib")
#pragma comment(lib, "..\\cppkore_libs\\LZHAM\\lzhamcomp_x64.lib")
#pragma comment(lib, "..\\cppkore_libs\\LZHAM\\lzhamdecomp_x64.lib")
#endif
#else
#error LZHAM only supported via x64 builds...
#endif

namespace Compression
{
	uint64_t LZHAMCodec::Compress(uint8_t* Input, uint64_t InputOffset, uint64_t InputLength, uint8_t* Output, uint64_t OutputOffset, uint64_t OutputLength, uint32_t DictSize)
	{
		uint64_t OutputSize = OutputLength;

		lzham_compress_params comp_params{};
		comp_params.m_struct_size = sizeof(comp_params);
		comp_params.m_dict_size_log2 = DictSize;
		comp_params.m_level = lzham_compress_level::LZHAM_COMP_LEVEL_DEFAULT;
		comp_params.m_max_helper_threads = 1;

		lzham_uint32 Adler = 0;
		lzham_compress_memory(&comp_params, (lzham_uint8*)(Output + OutputOffset), (size_t*)&OutputSize, (const lzham_uint8*)(Input + InputOffset), (size_t)InputLength, &Adler);

		return OutputSize;
	}

	std::unique_ptr<uint8_t[]> LZHAMCodec::Compress(uint8_t* Input, uint64_t InputOffset, uint64_t InputLength, uint64_t& OutputLength, uint32_t DictSize)
	{
		auto ResultBounds = lzham_z_compressBound((lzham_z_ulong)InputLength);
		auto Result = std::make_unique<uint8_t[]>(ResultBounds);

		OutputLength = ResultBounds;

		lzham_compress_params comp_params{};
		comp_params.m_struct_size = sizeof(comp_params);
		comp_params.m_dict_size_log2 = DictSize;
		comp_params.m_level = lzham_compress_level::LZHAM_COMP_LEVEL_DEFAULT;
		comp_params.m_max_helper_threads = 1;

		lzham_uint32 Adler = 0;

		if (lzham_compress_memory(&comp_params, (lzham_uint8*)(Result.get()), (size_t*)&OutputLength, (const lzham_uint8*)(Input + InputOffset), (size_t)InputLength, &Adler) != lzham_compress_status_t::LZHAM_COMP_STATUS_SUCCESS)
		{
			OutputLength = 0;
			return nullptr;
		}

		return Result;
	}

	uint64_t LZHAMCodec::Decompress(uint8_t* Input, uint64_t InputOffset, uint64_t InputLength, uint8_t* Output, uint64_t OutputOffset, uint64_t OutputLength, uint32_t DictSize)
	{
		uint64_t OutputSize = OutputLength;

		lzham_decompress_params dec_params{};
		dec_params.m_struct_size = sizeof(dec_params);
		dec_params.m_dict_size_log2 = DictSize;

		lzham_uint32 Adler = 0;
		lzham_decompress_memory(&dec_params, (lzham_uint8*)(Output + OutputOffset), (size_t*)&OutputSize, (const lzham_uint8*)(Input + InputOffset), (size_t)InputLength, &Adler);

		return OutputSize;
	}

	std::unique_ptr<uint8_t[]> LZHAMCodec::Decompress(uint8_t* Input, uint64_t InputOffset, uint64_t InputLength, uint64_t KnownOutputLength, uint32_t DictSize)
	{
		auto ResultSize = KnownOutputLength;
		auto Result = std::make_unique<uint8_t[]>(KnownOutputLength);

		lzham_decompress_params dec_params{};
		dec_params.m_struct_size = sizeof(dec_params);
		dec_params.m_dict_size_log2 = DictSize;

		lzham_uint32 Adler = 0;
		
		if (lzham_decompress_memory(&dec_params, (lzham_uint8*)(Result.get()), (size_t*)&ResultSize, (const lzham_uint8*)(Input + InputOffset), (size_t)InputLength, &Adler) != lzham_decompress_status_t::LZHAM_DECOMP_STATUS_SUCCESS)
			return nullptr;

		return Result;
	}
}
