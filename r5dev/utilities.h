#pragma once
#include <Windows.h>
#include <stdio.h>
#include <string>

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

static void HexDump(char* data, int len)
{
    // todo..
} 
