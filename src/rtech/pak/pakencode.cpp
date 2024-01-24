//=============================================================================//
//
// Purpose: buffered pak encoder
//
//=============================================================================//
#include "thirdparty/zstd/zstd.h"
#include "rtech/ipakfile.h"
#include "pakencode.h"
#include <filesystem/filesystem.h>

extern CFileSystem_Stdio* FileSystem();

//-----------------------------------------------------------------------------
// encodes the pak file from buffer, we can't do streamed compression as we
// need to know the actual decompress size ahead of time, else the runtime will
// fail as we wouldn't be able to parse the decompressed size from the frame
// header
//-----------------------------------------------------------------------------
bool Pak_EncodePak(const uint8_t* const inBuf, const uint64_t inLen,
	uint8_t* const outBuf, const uint64_t outLen, const int level)
{
	// offset to the actual pak data, the main file header shouldn't be
	// compressed
	const size_t dataOffset = sizeof(PakFileHeader_t);

	const size_t compressSize = ZSTD_compress(
		outBuf + dataOffset,
		outLen - dataOffset,
		inBuf + dataOffset,
		inLen - dataOffset,
		level);

	if (ZSTD_isError(compressSize))
		return false;

	const PakFileHeader_t* const inHeader = reinterpret_cast<const PakFileHeader_t* const>(inBuf);
	PakFileHeader_t* const outHeader = reinterpret_cast<PakFileHeader_t* const>(outBuf);

	// copy the header over
	*outHeader = *inHeader;

	// the compressed size includes the entire buffer, even the data we didn't
	// compress like the file header
	outHeader->compressedSize = compressSize + dataOffset;

	// these flags are required for the game's runtime to decide whether or not to
	// decompress the pak, and how; see Pak_ProcessPakFile() for more details
	outHeader->flags |= PAK_HEADER_FLAGS_COMPRESSED;
	outHeader->flags |= PAK_HEADER_FLAGS_ZSTD;

	return true;
}

//-----------------------------------------------------------------------------
// encodes the pak file from file name
//-----------------------------------------------------------------------------
bool Pak_EncodePakFile(const char* const inPakFile, const char* const outPakFile, const int level)
{
	FileHandle_t inFileHandle = FileSystem()->Open(inPakFile, "rb", "GAME");

	// unable to open
	if (!inFileHandle)
		return false;

	const ssize_t fileSize = FileSystem()->Size(inFileHandle);

	// corrupted/truncated
	if (fileSize <= sizeof(PakFileHeader_t))
	{
		FileSystem()->Close(inFileHandle);
		return false;
	}

	std::unique_ptr<uint8_t[]> pakBuf(new uint8_t[fileSize]);
	uint8_t* const pPakBuf = pakBuf.get();

	FileSystem()->Read(pPakBuf, fileSize, inFileHandle);
	FileSystem()->Close(inFileHandle);

	const PakFileHeader_t* inPakHeader = reinterpret_cast<PakFileHeader_t* const>(pPakBuf);

	// incompatible pak, or not a pak
	if (inPakHeader->magic != PAK_HEADER_MAGIC || inPakHeader->version != PAK_HEADER_VERSION)
		return false;

	// already compressed
	if (inPakHeader->IsCompressed())
		return false;

	std::unique_ptr<uint8_t[]> outBuf(new uint8_t[inPakHeader->decompressedSize]);
	uint8_t* const pOutBuf = outBuf.get();

	// encoding failed
	if (!Pak_EncodePak(pPakBuf, fileSize, pOutBuf, inPakHeader->decompressedSize, level))
		return false;

	FileHandle_t outFileHandle = FileSystem()->Open(outPakFile, "wb", "GAME");

	// failure to open output file
	if (!outFileHandle)
		return false;

	const PakFileHeader_t* outPakHeader = reinterpret_cast<PakFileHeader_t* const>(pOutBuf);

	// this will be true if the entire buffer has been written
	const bool ret = FileSystem()->Write(pOutBuf, outPakHeader->compressedSize, outFileHandle) == (ssize_t)outPakHeader->compressedSize;
	FileSystem()->Close(inFileHandle);

	return ret;
}
