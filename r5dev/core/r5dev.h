#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>

__declspec(dllexport) void DummyExport()
{
    // Required for detours.
}

const std::string R5R_LOGO[] =
{
    R"(+---------------------------------------------+)",
    R"(|  ___ ___ ___     _              _        _  |)",
    R"(| | _ \ __| _ \___| |___  __ _ __| |___ __| | |)",
    R"(| |   /__ \   / -_) / _ \/ _` / _` / -_) _` | |)",
    R"(| |_|_\___/_|_\___|_\___/\__,_\__,_\___\__,_| |)",
    R"(|                                             |)",
    R"(+---------------------------------------------+)"
};
