#pragma once
#include <stdio.h>
#include <string>

#include "hooks.h"

namespace
{
    BOOL FileExists(LPCTSTR szPath)
    {
        DWORD dwAttrib = GetFileAttributes(szPath);

        return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
            !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
    }

    static void DbgPrint(LPCSTR Format, ...)
    {
        CHAR Buffer[512] = { 0 };
        va_list Args;

        // Get the variable arg pointer.
        va_start(Args, Format);

        // Format print the string.
        int length = vsnprintf(Buffer, sizeof(Buffer), Format, Args);
        va_end(Args);

        // Output the string to the debugger.
        OutputDebugString(Buffer);
    }

    static void HexDump(const char* header, const char* file, const char* mode, int func, const void* data, int size)
    {
        // todo..
    }
}