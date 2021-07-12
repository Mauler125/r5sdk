#include <Windows.h>
#include <Psapi.h>
#include <stdio.h>

#include "utility.h"
#include "spdlog.h"
#include "hooks.h"

/*-----------------------------------------------------------------------------
 * _utility.cpp
 *-----------------------------------------------------------------------------*/

 //////////////////////////////////////////////////////////////////////////////
 //
BOOL FileExists(LPCTSTR szPath)
{
    DWORD dwAttrib = GetFileAttributes(szPath);

    return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
        !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

///////////////////////////////////////////////////////////////////////////////
// For getting information about the specified module
MODULEINFO GetModuleInfo(const char* szModule)
{
    MODULEINFO modinfo = { 0 };
    HMODULE hModule = GetModuleHandle(szModule);
    if (hModule == 0)
    {
        return modinfo;
    }
    GetModuleInformation(GetCurrentProcess(), hModule, &modinfo, sizeof(MODULEINFO));
    return modinfo;
}

///////////////////////////////////////////////////////////////////////////////
// For finding a byte pattern in memory of the game process
BOOL Compare(const unsigned char* pData, const unsigned char* szPattern, const char* szMask)
{
    for (; *szMask; ++szMask, ++pData, ++szPattern)
    {
        if (*szMask == 'x' && *pData != *szPattern)
        {
            return false;
        }
    }
    return (*szMask) == NULL;
}
DWORD64 FindPattern(const char* szModule, const unsigned char* szPattern, const char* szMask)
{
    MODULEINFO mInfo = GetModuleInfo(szModule);
    DWORD64 dwAddress = (DWORD64)mInfo.lpBaseOfDll;
    DWORD64 dwLen = (DWORD64)mInfo.SizeOfImage;

    size_t maskLen = strlen(szMask);
    for (int i = 0; i < dwLen - maskLen; i++)
    {
        if (Compare((unsigned char*)(dwAddress + i), szPattern, szMask))
        {
            return (DWORD64)(dwAddress + i);
        }
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
//
void DbgPrint(LPCSTR sFormat, ...)
{
    CHAR sBuffer[512] = { 0 };
    va_list sArgs;

    // Get the variable arg pointer
    va_start(sArgs, sFormat);

    // Format print the string
    int length = vsnprintf(sBuffer, sizeof(sBuffer), sFormat, sArgs);
    va_end(sArgs);

    // Output the string to the debugger
    OutputDebugString(sBuffer);
}

///////////////////////////////////////////////////////////////////////////////
// For dumping data from a buffer to a file on the disk
void HexDump(const char* szHeader, int nFunc, const void* pData, int nSize)
{
    static std::atomic<int> i, j, k = 0;
    static char ascii[17] = { 0 };
    static auto logger = spdlog::get("default_logger");
    auto pattern = std::make_unique<spdlog::pattern_formatter>("%v", spdlog::pattern_time_type::local, std::string(""));

    // Loop until the function returned to the first caller
    while (k == 1) { /*Sleep(75);*/ }

    k = 1;
    ascii[16] = '\0';

    // Add new loggers here to replace the placeholder
    if (nFunc == 0) { logger = g_spdnetchan_logger; }

    // Add timestamp
    logger->set_level(spdlog::level::trace);
    logger->set_pattern("%v [%H:%M:%S.%f]");
    logger->trace("---------------------------------------------------------");

    // Disable EOL and create block header
    logger->set_formatter(std::move(pattern));
    logger->trace("{:s} ---- LEN BYTES: {}\n:\n", szHeader, nSize);
    logger->trace("--------  0  1  2  3  4  5  6  7   8  9  A  B  C  D  E  F  0123456789ABCDEF\n");

    // Output the buffer to the file
    for (i = 0; i < nSize; i++)
    {
        if (i % nSize == 0) { logger->trace(" 0x{:04X}  ", i); }
        logger->trace("{:02x} ", ((unsigned char*)pData)[i]);

        if (((unsigned char*)pData)[i] >= ' ' && ((unsigned char*)pData)[i] <= '~') { ascii[i % 16] = ((unsigned char*)pData)[i]; }
        else { ascii[i % 16] = '.'; }

        if ((i + 1) % 8 == 0 || i + 1 == nSize)
        {
            logger->trace(" ");

            if ((i + 1) % 16 == 0)
            {
                if (i + 1 == nSize)
                {
                    logger->trace("{:s}\n", ascii);
                    logger->trace("---------------------------------------------------------------------------\n");
                    logger->trace("\n");
                }
                else
                {
                    i++;
                    logger->trace("{:s}\n ", ascii);
                    logger->trace("0x{:04X}  ", i--);
                }
            }
            else if (i + 1 == nSize)
            {
                ascii[(i + 1) % 16] = '\0';
                if ((i + 1) % 16 <= 8)
                {
                    logger->trace(" ");
                }
                for (j = (i + 1) % 16; j < 16; j++)
                {
                    logger->trace("   ");
                }
                logger->trace("{:s}\n", ascii);
                logger->trace("---------------------------------------------------------------------------\n");
                logger->trace("\n");
            }
        }
    }
    k = 0;
    ///////////////////////////////////////////////////////////////////////////
}