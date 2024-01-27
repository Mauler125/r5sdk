//=============================================================================//
//
// Purpose: buffered pak encoder
//
//=============================================================================//
#include "tier0/binstream.h"
#include "thirdparty/zstd/zstd.h"
#include "rtech/ipakfile.h"
#include "paktools.h"
#include "pakencode.h"

//-----------------------------------------------------------------------------
// encodes the pak file from buffer, we can't do streamed compression as we
// need to know the actual decompress size ahead of time, else the runtime will
// fail as we wouldn't be able to parse the decompressed size from the frame
// header
//-----------------------------------------------------------------------------
bool Pak_BufferToBufferEncode(const uint8_t* const inBuf, const uint64_t inLen,
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

	PakFileHeader_t* const outHeader = reinterpret_cast<PakFileHeader_t* const>(outBuf);

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
// searches for pak patches and updates all referenced files in patch headers
//-----------------------------------------------------------------------------
bool Pak_UpdatePatchHeaders(uint8_t* const inBuf, const char* const outPakFile)
{
	// file name without extension and patch number ID
	char baseFilePath[MAX_PATH];
	snprintf(baseFilePath, sizeof(baseFilePath), "%s", outPakFile);

	const char* const fileNameUnqualified = V_UnqualifiedFileName(baseFilePath);

	V_StripExtension(baseFilePath, baseFilePath, sizeof(baseFilePath));

	// strip the patch id, but make sure we only do this on the file name
	// and not the path
	char* const patchIdentifier = strrchr(baseFilePath, '(');

	if (patchIdentifier && patchIdentifier > fileNameUnqualified)
		*patchIdentifier = '\0';

	// NOTE: we modify the in buffer as the patch headers belong to the
	// compressed section
	PakFileHeader_t* const inHeader = reinterpret_cast<PakFileHeader_t* const>(inBuf);

	// update each patch header
	for (uint16_t i = 0; i < inHeader->patchIndex; i++)
	{
		short patchNumber = Pak_GetPatchNumberForIndex(inHeader, i);
		char patchFile[MAX_PATH];

		// the first patch number does not have an identifier in its name
		if (patchNumber == 0)
			snprintf(patchFile, sizeof(patchFile), "%s.rpak", baseFilePath);
		else
			snprintf(patchFile, sizeof(patchFile), "%s(%02u).rpak", baseFilePath, patchNumber);

		CIOStream inPatch;

		// unable to open patch while there should be one, we must calculate
		// new file sizes here, or else the runtime would fail to load them
		if (!inPatch.Open(patchFile, CIOStream::READ | CIOStream::BINARY))
			return false;

		const size_t fileSize = inPatch.GetSize();

		// pak appears truncated
		if (fileSize <= sizeof(PakFileHeader_t))
			return false;

		DevMsg(eDLL_T::RTECH, "%s: updating patch header for pak '%s', new size = %zu\n",
			__FUNCTION__, patchFile, fileSize);

		PakPatchFileHeader_t* const patchHeader = Pak_GetPatchFileHeader(inHeader, i);
		patchHeader->m_sizeDisk = fileSize;
	}

	return true;
}

//-----------------------------------------------------------------------------
// encodes the pak file from file name
//-----------------------------------------------------------------------------
bool Pak_EncodePakFile(const char* const inPakFile, const char* const outPakFile, const int level)
{
	CIOStream inFile;

	// failed to open
	if (!inFile.Open(inPakFile, CIOStream::READ | CIOStream::BINARY))
		return false;

	const ssize_t fileSize = inFile.GetSize();

	// file appears truncated
	if (fileSize <= sizeof(PakFileHeader_t))
		return false;

	std::unique_ptr<uint8_t[]> pakBuf(new uint8_t[fileSize]);
	uint8_t* const pPakBuf = pakBuf.get();

	inFile.Read(pPakBuf, fileSize);
	inFile.Close();

	const PakFileHeader_t* inHeader = reinterpret_cast<PakFileHeader_t* const>(pPakBuf);

	// incompatible pak, or not a pak
	if (inHeader->magic != PAK_HEADER_MAGIC || inHeader->version != PAK_HEADER_VERSION)
		return false;

	// already compressed
	if (inHeader->IsCompressed())
		return false;

	if (inHeader->patchIndex && !Pak_UpdatePatchHeaders(pPakBuf, outPakFile))
	{
		// pak has patches but one or more weren't found; the headers need to
		// be updated with new compression sizes since the runtime uses them to
		// check for truncation
		return false;
	}

	std::unique_ptr<uint8_t[]> outBuf(new uint8_t[inHeader->decompressedSize]);
	uint8_t* const pOutBuf = outBuf.get();

	PakFileHeader_t* const outHeader = reinterpret_cast<PakFileHeader_t* const>(pOutBuf);

	// copy the header over
	*outHeader = *inHeader;

	// encoding failed
	if (!Pak_BufferToBufferEncode(pPakBuf, fileSize, pOutBuf, inHeader->decompressedSize, level))
		return false;

	CIOStream outFile;

	// failure to open output file
	if (!outFile.Open(outPakFile, CIOStream::WRITE | CIOStream::BINARY))
		return false;

	const PakFileHeader_t* outPakHeader = reinterpret_cast<PakFileHeader_t* const>(pOutBuf);

	// this will be true if the entire buffer has been written
	outFile.Write(pOutBuf, outPakHeader->compressedSize);
	return true;
}
