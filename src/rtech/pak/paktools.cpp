//=============================================================================//
//
// Purpose: general pak tools
//
//=============================================================================//
#include "rtech/ipakfile.h"
#include "pakstate.h"
#include "paktools.h"

//----------------------------------------------------------------------------------
// checks if an override file exists and returns whether it should be loaded instead
//----------------------------------------------------------------------------------
bool Pak_GetOverride(const char* const pakFilePath, char* const outPath, const size_t outBufLen)
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
    if (Pak_GetOverride(pakFilePath, fullPath, sizeof(fullPath)))
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
const PakLoadedInfo_t* Pak_GetPakInfo(const PakHandle_t pakId)
{
	for (int16_t i = 0; i < *g_pLoadedPakCount; ++i)
	{
		const PakLoadedInfo_t* const info = &g_pLoadedPakInfo[i];
		if (!info)
			continue;

		if (info->handle != pakId)
			continue;

		return info;
	}

	Warning(eDLL_T::RTECH, "%s - Failed to retrieve pak info for handle '%d'\n", __FUNCTION__, pakId);
	return nullptr;
}

//-----------------------------------------------------------------------------
// gets information about loaded pak file via pak name
//-----------------------------------------------------------------------------
const PakLoadedInfo_t* Pak_GetPakInfo(const char* const pakName)
{
	for (int16_t i = 0; i < *g_pLoadedPakCount; ++i)
	{
		const PakLoadedInfo_t* const info = &g_pLoadedPakInfo[i];
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
	const uint8_t* patchHeader = reinterpret_cast<const uint8_t*>(Pak_GetPatchFileHeader(pakHeader, pakHeader->patchIndex - 1));

	// skip the last patch file header, the patch number start from there
	patchHeader += sizeof(PakPatchFileHeader_t);
	const short* patchNumber = reinterpret_cast<const short*>(patchHeader);

	return patchNumber[index];
}
