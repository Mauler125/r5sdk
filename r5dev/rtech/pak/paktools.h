#ifndef RTECH_PAKTOOLS_H
#define RTECH_PAKTOOLS_H
#include "rtech/ipakfile.h"

extern bool Pak_GetOverride(const char* const pakFilePath, char* const outPath, const size_t outBufLen);
extern int Pak_FileExists(const char* const pakFilePath);

extern const char* Pak_StatusToString(const EPakStatus status);
extern PakGuid_t Pak_StringToGuid(const char* const string);

extern const PakLoadedInfo_t* Pak_GetPakInfo(const PakHandle_t pakId);
extern const PakLoadedInfo_t* Pak_GetPakInfo(const char* const pakName);

extern PakPatchDataHeader_t* Pak_GetPatchDataHeader(PakFileHeader_t* const pakHeader);
extern PakPatchFileHeader_t* Pak_GetPatchFileHeader(PakFileHeader_t* const pakHeader, const int index);
extern short Pak_GetPatchNumberForIndex(PakFileHeader_t* const pakHeader, const int index);

#endif // !RTECH_PAKTOOLS_H
