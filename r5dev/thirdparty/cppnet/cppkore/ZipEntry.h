#pragma once

#include <cstdint>
#include "StringBase.h"

namespace Compression
{
	// Compression method enumeration
	enum class ZipCompressionMethod : uint16_t
	{
		// Uncompressed storage
		Store = 0,
		// Deflate compression method
		Deflate = 8
	};

	// Represents an entry in the ZipArchive
	struct ZipEntry
	{
		// Compression method
		ZipCompressionMethod Method;
		// Full path and filename as stored in ZipArchive
		string FileNameInZip;
		// Original file size
		uint64_t FileSize;
		// Compressed file size
		uint64_t CompressedSize;
		// Offset of header information in the ZipArchive
		uint64_t HeaderOffset;
		// Offset of file inside the ZipArchive
		uint64_t FileOffset;
		// Size of the header information
		uint32_t HeaderSize;
		// The CRC32 checksum of the entire file
		uint32_t Crc32;
		// User comment for the file
		string Comment;
		// True if UTF8 encoding for filename and comments
		bool EncodeUTF8;
	};
}