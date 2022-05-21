#pragma once

#include <cstdint>

namespace Compression
{
	// Which mode of operation we are performing
	enum class CompressionMode
	{
		Compress,
		Decompress
	};
}