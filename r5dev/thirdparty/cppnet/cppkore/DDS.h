#pragma once

#include <cstdint>
#include <memory>
#include <dxgiformat.h>
#include "Stream.h"

namespace Assets
{
	// Flags used to extend the DDS texture format.
	enum class DDSFormatFlags
	{
		None = 0
	};

	// Contains information about the DDS texture format.
	struct DDSFormat
	{
		// The count of mip levels in this texture (Default: 1) (>= 1)
		uint32_t MipLevels;
		// Whether or not this is a cubemapped texture. (Default: false)
		bool CubeMap;
		// The block compression format for the texture. (Default: DXGI_FORMAT_UNKNOWN)
		DXGI_FORMAT Format;
		// Flags which extend the format for the texture. (Default: None)
		DDSFormatFlags Flags;

		DDSFormat();
	};

	// A utility class for building DDS assets.
	class DDS
	{
		// Don't initialize this class
		DDS() = delete;
		~DDS() = delete;

	public:
		// Transcodes a buffer of (legacy) RGB8 data to a DXGI compatible RGBA buffer
		static std::unique_ptr<uint8_t[]> TranscodeRGBToRGBA(const uint8_t* Buffer, uint32_t BufferSize, uint64_t& ResultSize);

		// Serializes a DDS header to the given stream.
		static void WriteDDSHeader(const std::unique_ptr<IO::Stream>& Stream, uint32_t Width, uint32_t Height, const DDSFormat& Format);
		// Serializes a DDS header to the given stream.
		static void WriteDDSHeader(IO::Stream* Stream, uint32_t Width, uint32_t Height, const DDSFormat& Format);

		// Serializes a DDS header to the buffer.
		static void WriteDDSHeader(uint8_t* Buffer, uint32_t Width, uint32_t Height, const DDSFormat& Format, uint32_t& ResultSize);

		// Calculate a block size for the given format.
		static const uint32_t CalculateBlockSize(uint32_t Width, uint32_t Height, const DDSFormat& Format);
		// Calculate the maximum level of mips.
		static const uint32_t CountMipLevels(uint32_t Width, uint32_t Height);
	};
}