#pragma once
#include <Windows.h>
#include <stdio.h>
#include <string>

namespace
{
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

    typedef unsigned __int64 QWORD;

    BOOL FileExists(LPCTSTR szPath)
    {
        DWORD dwAttrib = GetFileAttributes(szPath);

        return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
            !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
    }

    static void HexDump(char* data, int len)
    {
        // todo..
    }
}