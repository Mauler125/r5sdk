#pragma once
#include <thirdparty/spdlog/include/sinks/basic_file_sink.h>

/////////////////////////////////////////////////////////////////////////////
// Internals
BOOL IsBadReadPtrV2(void* ptr);
BOOL FileExists(const char* szPath);
MODULEINFO GetModuleInfo(const char* szModule);
DWORD64 FindPatternSIMD(const char* szModule, const unsigned char* szPattern, const char* szMask);

/////////////////////////////////////////////////////////////////////////////
// Utility
void DbgPrint(LPCSTR sFormat, ...);
void HexDump(const char* szHeader, int nFunc, const void* pData, int nSize);
std::string Base64Encode(const std::string& in);
std::string Base64Decode(const std::string& in);
bool StringReplace(std::string& str, const std::string& from, const std::string& to);
std::string CreateDirectories(std::string svFilePath);

/////////////////////////////////////////////////////////////////////////////
