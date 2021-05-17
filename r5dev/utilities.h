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

    /////////////////////////////////////////////////////////////////////////////
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

    /////////////////////////////////////////////////////////////////////////////
    static void HexDump(const char* header, const char* file, const char* mode, int func, const void* data, int size)
    {
        char ascii[17] = { 0 };
        int i, j;
        ascii[16] = '\0';
        FILE* sTraceLog;

#pragma warning(suppress : 4996)
        sTraceLog = fopen(file, mode);
        if (sTraceLog == NULL)
        {
            printf("Unable to write '%s'!\n", file);
            if (func == 0) { ToggleNetHooks(); }
            return;
        }

        // Create block header
        fprintf(sTraceLog, "%s ---- %u Bytes\n:\n", header, size);
        fprintf(sTraceLog, "--------  0  1  2  3  4  5  6  7   8  9  A  B  C  D  E  F  0123456789ABCDEF\n");

        // Output the buffer to the file
        for (i = 0; i < size; i++)
        {
            if (i % size == 0) { fprintf(sTraceLog, " 0x%04x  ", i); fflush(NULL); }
            fprintf(sTraceLog, "%02x ", ((unsigned char*)data)[i]);
            fflush(NULL);

            if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') { ascii[i % 16] = ((unsigned char*)data)[i]; }
            else { ascii[i % 16] = '.'; }

            if ((i + 1) % 8 == 0 || i + 1 == size)
            {
                fprintf(sTraceLog, " ");
                fflush(NULL);

                if ((i + 1) % 16 == 0) { i++; fprintf(sTraceLog, "%s \n ", ascii); fprintf(sTraceLog, "0x%04X  ", i--); fflush(NULL); }
                else if (i + 1 == size)
                {
                    ascii[(i + 1) % 16] = '\0';
                    if ((i + 1) % 16 <= 8) { fprintf(sTraceLog, " "); fflush(NULL); }
                    for (j = (i + 1) % 16; j < 16; j++) { fprintf(sTraceLog, "   "); fflush(NULL); }
                    fprintf(sTraceLog, "%s \n", ascii);
                    fprintf(sTraceLog, "---------------------------------------------------------------------------\n\n");
                }
            }
        }

        fclose(sTraceLog);
    }
}