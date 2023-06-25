#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>

__declspec(dllexport) void DummyExport()
{
    // Required for detours.
}

static const char* const R5R_EMBLEM[] =
{
    R"(+-------------------------------------------------------------+)",
    R"(|  ___ ___ ___     _              _        _       ___   ___  |)",
    R"(| | _ \ __| _ \___| |___  __ _ __| |___ __| | __ _|_  ) |_  ) |)",
    R"(| |   /__ \   / -_) / _ \/ _` / _` / -_) _` | \ V // / _ / /  |)",
    R"(| |_|_\___/_|_\___|_\___/\__,_\__,_\___\__,_|  \_//___(_)___| |)",
    R"(|                                                             |)"/*,
    R"(+-------------------------------------------------------------+)"*/
};
