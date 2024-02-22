//=============================================================================//
//
// Purpose: general pak tools
//
//=============================================================================//
#include "tier0/binstream.h"
#include "rtech/ipakfile.h"

#include "pakstate.h"
#include "paktools.h"

//----------------------------------------------------------------------------------
// checks if the pak base path exists
//----------------------------------------------------------------------------------
bool Pak_BasePathExists()
{
	return IsDirectory(PLATFORM_PAK_PATH);
}

//----------------------------------------------------------------------------------
// creates the pak base path if it doesn't exists
//----------------------------------------------------------------------------------
bool Pak_CreateBasePath()
{
	// already exists
	if (Pak_BasePathExists())
		return true;

	return CreateDirHierarchy(PLATFORM_PAK_PATH) == 0;
}

//----------------------------------------------------------------------------------
// checks if the pak override path exists
//----------------------------------------------------------------------------------
bool Pak_OverridePathExists()
{
	return IsDirectory(PLATFORM_PAK_OVERRIDE_PATH);
}

//----------------------------------------------------------------------------------
// creates the pak override path if it doesn't exists
//----------------------------------------------------------------------------------
bool Pak_CreateOverridePath()
{
	// already exists
	if (Pak_OverridePathExists())
		return true;

	return CreateDirHierarchy(PLATFORM_PAK_OVERRIDE_PATH) == 0;
}

//----------------------------------------------------------------------------------
// checks if an override file exists and returns whether it should be loaded instead
//----------------------------------------------------------------------------------
bool Pak_FileOverrideExists(const char* const pakFilePath, char* const outPath, const size_t outBufLen)
{
    // check the overrides path
    snprintf(outPath, outBufLen, PLATFORM_PAK_OVERRIDE_PATH"%s", V_UnqualifiedFileName(pakFilePath));
    return FileExists(outPath);
}

//----------------------------------------------------------------------------------
// checks if a pak file exists
//----------------------------------------------------------------------------------
int Pak_FileExists(const char* const pakFilePath)
{
    char fullPath[1024];
    if (Pak_FileOverrideExists(pakFilePath, fullPath, sizeof(fullPath)))
        return true;

    // check the platform's default path
    snprintf(fullPath, sizeof(fullPath), PLATFORM_PAK_PATH"%s", pakFilePath);

    return FileExists(fullPath);
}

//-----------------------------------------------------------------------------
// returns pak status as string
//-----------------------------------------------------------------------------
const char* Pak_StatusToString(const EPakStatus status)
{
	switch (status)
	{
	case EPakStatus::PAK_STATUS_FREED:                  return "PAK_STATUS_FREED";
	case EPakStatus::PAK_STATUS_LOAD_PENDING:           return "PAK_STATUS_LOAD_PENDING";
	case EPakStatus::PAK_STATUS_REPAK_RUNNING:          return "PAK_STATUS_REPAK_RUNNING";
	case EPakStatus::PAK_STATUS_REPAK_DONE:             return "PAK_STATUS_REPAK_DONE";
	case EPakStatus::PAK_STATUS_LOAD_STARTING:          return "PAK_STATUS_LOAD_STARTING";
	case EPakStatus::PAK_STATUS_LOAD_PAKHDR:            return "PAK_STATUS_LOAD_PAKHDR";
	case EPakStatus::PAK_STATUS_LOAD_PATCH_INIT:        return "PAK_STATUS_LOAD_PATCH_INIT";
	case EPakStatus::PAK_STATUS_LOAD_PATCH_EDIT_STREAM: return "PAK_STATUS_LOAD_PATCH_EDIT_STREAM";
	case EPakStatus::PAK_STATUS_LOAD_ASSETS:            return "PAK_STATUS_LOAD_ASSETS";
	case EPakStatus::PAK_STATUS_LOADED:                 return "PAK_STATUS_LOADED";
	case EPakStatus::PAK_STATUS_UNLOAD_PENDING:         return "PAK_STATUS_UNLOAD_PENDING";
	case EPakStatus::PAK_STATUS_FREE_PENDING:           return "PAK_STATUS_FREE_PENDING";
	case EPakStatus::PAK_STATUS_CANCELING:              return "PAK_STATUS_CANCELING";
	case EPakStatus::PAK_STATUS_ERROR:                  return "PAK_STATUS_ERROR";
	case EPakStatus::PAK_STATUS_INVALID_PAKHANDLE:      return "PAK_STATUS_INVALID_PAKHANDLE";
	case EPakStatus::PAK_STATUS_BUSY:                   return "PAK_STATUS_BUSY";
	default:                                            return "PAK_STATUS_UNKNOWN";
	}
}

//-----------------------------------------------------------------------------
// returns pak decoder as string
//-----------------------------------------------------------------------------
const char* Pak_DecoderToString(const EPakDecodeMode mode)
{
	switch (mode)
	{
	case EPakDecodeMode::MODE_RTECH: return "RTech";
	case EPakDecodeMode::MODE_ZSTD: return "ZStd";
	}

	UNREACHABLE();
}

//-----------------------------------------------------------------------------
// compute a guid from input string data
//-----------------------------------------------------------------------------
PakGuid_t Pak_StringToGuid(const char* const string)
{
	uint32_t* v1; // r8
	uint64_t         v2; // r10
	int32_t          v3; // er11
	uint32_t         v4; // er9
	uint32_t          i; // edx
	uint64_t         v6; // rcx
	int32_t          v7; // er9
	int32_t          v8; // edx
	int32_t          v9; // eax
	uint32_t        v10; // er8
	int32_t         v12; // ecx
	uint32_t* a1 = (uint32_t*)string;

	v1 = a1;
	v2 = 0;
	v3 = 0;
	v4 = (*a1 - 45 * ((~(*a1 ^ 0x5C5C5C5Cu) >> 7) & (((*a1 ^ 0x5C5C5C5Cu) - 0x1010101) >> 7) & 0x1010101)) & 0xDFDFDFDF;
	for (i = ~*a1 & (*a1 - 0x1010101) & 0x80808080; !i; i = v8 & 0x80808080)
	{
		v6 = v4;
		v7 = v1[1];
		++v1;
		v3 += 4;
		v2 = ((((uint64_t)(0xFB8C4D96501i64 * v6) >> 24) + 0x633D5F1 * v2) >> 61) ^ (((uint64_t)(0xFB8C4D96501i64 * v6) >> 24)
			+ 0x633D5F1 * v2);
		v8 = ~v7 & (v7 - 0x1010101);
		v4 = (v7 - 45 * ((~(v7 ^ 0x5C5C5C5Cu) >> 7) & (((v7 ^ 0x5C5C5C5Cu) - 0x1010101) >> 7) & 0x1010101)) & 0xDFDFDFDF;
	}
	v9 = -1;
	v10 = (i & -(signed)i) - 1;
	if (_BitScanReverse((unsigned long*)&v12, v10))
	{
		v9 = v12;
	}
	return 0x633D5F1 * v2 + ((0xFB8C4D96501i64 * (uint64_t)(v4 & v10)) >> 24) - 0xAE502812AA7333i64 * (uint32_t)(v3 + v9 / 8);
}

//-----------------------------------------------------------------------------
// gets information about loaded pak file via pak id
//-----------------------------------------------------------------------------
PakLoadedInfo_t* Pak_GetPakInfo(const PakHandle_t pakId)
{
	return &g_pakGlobals->loadedPaks[pakId & PAK_MAX_HANDLES_MASK];
}

//-----------------------------------------------------------------------------
// gets information about loaded pak file via pak name
//-----------------------------------------------------------------------------
const PakLoadedInfo_t* Pak_GetPakInfo(const char* const pakName)
{
	for (int16_t i = 0; i < g_pakGlobals->loadedPakCount; ++i)
	{
		const PakLoadedInfo_t* const info = &g_pakGlobals->loadedPaks[i];
		if (!info)
			continue;

		if (!info->fileName || !*info->fileName)
			continue;

		if (strcmp(pakName, info->fileName) != 0)
			continue;

		return info;
	}

	Warning(eDLL_T::RTECH, "%s - Failed to retrieve pak info for name '%s'\n", __FUNCTION__, pakName);
	return nullptr;
}

//-----------------------------------------------------------------------------
// returns a pointer to the patch data header
//-----------------------------------------------------------------------------
PakPatchDataHeader_t* Pak_GetPatchDataHeader(PakFileHeader_t* const pakHeader)
{
	// shouldn't be called if the pak doesn1't have patches!
	assert(pakHeader->patchIndex > 0);
	return reinterpret_cast<PakPatchDataHeader_t*>(reinterpret_cast<uint8_t* const>(pakHeader) + sizeof(PakFileHeader_t));
}

//-----------------------------------------------------------------------------
// returns a pointer to the patch file header
//-----------------------------------------------------------------------------
PakPatchFileHeader_t* Pak_GetPatchFileHeader(PakFileHeader_t* const pakHeader, const int index)
{
	assert(pakHeader->patchIndex > 0 && index < pakHeader->patchIndex);
	uint8_t* address = reinterpret_cast<uint8_t* const>(pakHeader);

	// skip the file header
	address += sizeof(PakFileHeader_t);

	// skip the patch data header, the patch file headers start from there
	address += sizeof(PakPatchDataHeader_t);
	PakPatchFileHeader_t* const patchHeaders = reinterpret_cast<PakPatchFileHeader_t* const>(address);

	return &patchHeaders[index];
}

//-----------------------------------------------------------------------------
// returns the patch number belonging to the index provided
//-----------------------------------------------------------------------------
short Pak_GetPatchNumberForIndex(PakFileHeader_t* const pakHeader, const int index)
{
	assert(pakHeader->patchIndex > 0 && index < pakHeader->patchIndex);
	const uint8_t* patchHeader = reinterpret_cast<const uint8_t*>(Pak_GetPatchFileHeader(pakHeader, pakHeader->patchIndex - 1));

	// skip the last patch file header, the patch number start from there
	patchHeader += sizeof(PakPatchFileHeader_t);
	const short* patchNumber = reinterpret_cast<const short*>(patchHeader);

	return patchNumber[index];
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
		patchHeader->compressedSize = fileSize;
	}

	return true;
}

//-----------------------------------------------------------------------------
// prints the pak header details to the console
//-----------------------------------------------------------------------------
void Pak_ShowHeaderDetails(const PakFileHeader_t* const pakHeader)
{
	SYSTEMTIME systemTime;
	FileTimeToSystemTime(&pakHeader->fileTime, &systemTime);

	Msg(eDLL_T::RTECH, "______________________________________________________________\n");
	Msg(eDLL_T::RTECH, "-+ Pak header details ----------------------------------------\n");
	Msg(eDLL_T::RTECH, " |-- Magic    : '0x%08X'\n", pakHeader->magic);
	Msg(eDLL_T::RTECH, " |-- Version  : '%hu'\n", pakHeader->version);
	Msg(eDLL_T::RTECH, " |-- Flags    : '0x%04hX'\n", pakHeader->flags);
	Msg(eDLL_T::RTECH, " |-- Time     : '%hu-%hu-%hu/%hu %hu:%hu:%hu.%hu'\n",
		systemTime.wYear, systemTime.wMonth, systemTime.wDay, systemTime.wDayOfWeek,
		systemTime.wHour, systemTime.wMinute, systemTime.wSecond, systemTime.wMilliseconds);
	Msg(eDLL_T::RTECH, " |-- Checksum : '0x%08llX'\n", pakHeader->checksum);
	Msg(eDLL_T::RTECH, " |-- Assets   : '%u'\n", pakHeader->assetCount);
	Msg(eDLL_T::RTECH, " |-+ Compression ---------------------------------------------\n");
	Msg(eDLL_T::RTECH, " | |-- Size comp: '%zu'\n", pakHeader->compressedSize);
	Msg(eDLL_T::RTECH, " | |-- Size decp: '%zu'\n", pakHeader->decompressedSize);
	Msg(eDLL_T::RTECH, " | |-- Ratio    : '%.02f'\n", (pakHeader->compressedSize * 100.f) / pakHeader->decompressedSize);
	Msg(eDLL_T::RTECH, "--------------------------------------------------------------\n");
}
