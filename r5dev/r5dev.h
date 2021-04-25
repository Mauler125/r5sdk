#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>

__declspec(dllexport) void DummyExport()
{
    // Required for detours.
}