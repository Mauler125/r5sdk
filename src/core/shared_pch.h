//===========================================================================//
//
// Purpose: Shared precompiled header file.
//
//===========================================================================//
#ifndef SHARED_PCH_H
#define SHARED_PCH_H

#if defined(_DEBUG) || defined(_PROFILE)
#pragma message ("Profiling is turned on; do not release this binary!\n")
#endif // _DEBUG || _PROFILE

// System includes.
#define WIN32_LEAN_AND_MEAN // Prevent winsock2 redefinition.
#include <windows.h>
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <bcrypt.h>
#include <comdef.h>
#include <direct.h>
#include <dbghelp.h>
#include <timeapi.h>
#include <shellapi.h>
#include <Psapi.h>
#include <setjmp.h>
#include <stdio.h>
#include <shlobj.h>
#include <objbase.h>
#include <intrin.h>
#include <emmintrin.h>
#include <cmath>
#include <cctype>
#include <cinttypes>
#include <regex>
#include <mutex>
#include <thread>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <filesystem>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <functional>

#include <smmintrin.h>

// Core includes.
#include "core/assert.h"
#include "core/termutil.h"

// Common includes.
#include "common/experimental.h"
#include "common/pseudodefs.h"
#include "common/x86defs.h"
#include "common/sdkdefs.h"

// Windows specifics, to support compiling the SDK with older versions of the Windows 10 SDK.
#ifndef FILE_SUPPORTS_GHOSTING
#define FILE_SUPPORTS_GHOSTING 0x40000000  // winnt
#endif
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

#define MAIN_WORKER_DLL "gamesdk.dll"
#define SERVER_WORKER_DLL "dedicated.dll"
#define CLIENT_WORKER_DLL "bin\\x64_retail\\client.dll"

#define MAIN_GAME_DLL "r5apex.exe"
#define SERVER_GAME_DLL "r5apex_ds.exe"

#endif // SHARED_PCH_H
