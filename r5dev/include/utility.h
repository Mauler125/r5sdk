#pragma once
#include <Windows.h>
#include <Psapi.h>
#include <sinks/basic_file_sink.h>

/////////////////////////////////////////////////////////////////////////////
// Internals
BOOL FileExists(LPCTSTR szPath);
MODULEINFO GetModuleInfo(const char* szModule);
DWORD64 FindPattern(const char* szModule, const unsigned char* szPattern, const char* szMask);

/////////////////////////////////////////////////////////////////////////////
// Utility
void DbgPrint(LPCSTR sFormat, ...);
void HexDump(const char* szHeader, int nFunc, const void* pData, int nSize);

/////////////////////////////////////////////////////////////////////////////
// Loggers
inline auto g_spddefault_logger = spdlog::basic_logger_mt("default_logger", "platform\\log\\default_r5.log");
inline auto g_spdnetchan_logger = spdlog::basic_logger_mt("netchan_logger", "platform\\log\\netchan_r5.log");

/////////////////////////////////////////////////////////////////////////////
