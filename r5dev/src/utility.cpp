#include <Windows.h>
#include <Psapi.h>
#include <stdio.h>

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
void HexDump(const char* szHeader, const char* szFile, const char* szMode, int nFunc, const void* pData, int nSize)
{
    char ascii[17] = { 0 };
    int i, j;
    ascii[16] = '\0';
    FILE* sTraceLog;

#pragma warning(suppress : 4996)
    sTraceLog = fopen(szFile, szMode);
    if (sTraceLog == NULL)
    {
        printf("Unable to write '%s'!\n", szFile);
        if (nFunc == 0) { ToggleNetHooks(); }
        return;
    }

    // Create block header
    fprintf(sTraceLog, "%s ---- %u Bytes\n:\n", szHeader, nSize);
    fprintf(sTraceLog, "--------  0  1  2  3  4  5  6  7   8  9  A  B  C  D  E  F  0123456789ABCDEF\n");

    // Output the buffer to the file
    for (i = 0; i < nSize; i++)
    {
        if (i % nSize == 0) { fprintf(sTraceLog, " 0x%04x  ", i); fflush(NULL); }
        fprintf(sTraceLog, "%02x ", ((unsigned char*)pData)[i]);
        fflush(NULL);

        if (((unsigned char*)pData)[i] >= ' ' && ((unsigned char*)pData)[i] <= '~') { ascii[i % 16] = ((unsigned char*)pData)[i]; }
        else { ascii[i % 16] = '.'; }

        if ((i + 1) % 8 == 0 || i + 1 == nSize)
        {
            fprintf(sTraceLog, " ");
            fflush(NULL);

            if ((i + 1) % 16 == 0) { i++; fprintf(sTraceLog, "%s \n ", ascii); fprintf(sTraceLog, "0x%04X  ", i--); fflush(NULL); }
            else if (i + 1 == nSize)
            {
                ascii[(i + 1) % 16] = '\0';
                if ((i + 1) % 16 <= 8) { fprintf(sTraceLog, " "); fflush(NULL); }
                for (j = (i + 1) % 16; j < 16; j++) { fprintf(sTraceLog, "   "); fflush(NULL); }
                fprintf(sTraceLog, "%s \n", ascii);
                fprintf(sTraceLog, "---------------------------------------------------------------------------\n\n");
            }
        }
    }
    ///////////////////////////////////////////////////////////////////////////
    fclose(sTraceLog);
}
