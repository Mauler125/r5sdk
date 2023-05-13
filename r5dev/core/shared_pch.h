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

#define WIN32_LEAN_AND_MEAN // Prevent winsock2 redefinition.
#include <windows.h>
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <bcrypt.h>
#include <comdef.h>
#include <direct.h>
#include <gdiplus.h>
#include <dbghelp.h>
#include <timeapi.h>
#include <shellapi.h>
#include <Psapi.h>
#include <setjmp.h>
#include <tchar.h>
#include <stdio.h>
#include <shlobj.h>
#include <objbase.h>
#include <intrin.h>
#include <emmintrin.h>
#include <cmath>
#include <cctype>
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

// Windows specifics, to support compiling the SDK with older versions of the Windows 10 SDK.
#ifndef FILE_SUPPORTS_GHOSTING
#define FILE_SUPPORTS_GHOSTING 0x40000000  // winnt
#endif
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

// Thirdparty includes.
#include "thirdparty/detours/include/detours.h"
#include "thirdparty/detours/include/idetour.h"

#include "thirdparty/lzham/include/lzham_assert.h"
#include "thirdparty/lzham/include/lzham_types.h"
#include "thirdparty/lzham/include/lzham.h"

#include "thirdparty/curl/include/curl/curl.h"

// Core includes.
#include "core/assert.h"
#include "core/termutil.h"

// Common includes.
#include "common/experimental.h"
#include "common/pseudodefs.h"
#include "common/x86defs.h"
#include "common/sdkdefs.h"

// Tier0 includes.
#include "tier0/utility.h"
#include "tier0/memaddr.h"
#include "tier0/module.h"
#include "tier0/basetypes.h"
#include "tier0/platform.h"
#include "tier0/annotations.h"
#include "tier0/commonmacros.h"
#include "tier0/memalloc.h"
#include "tier0/tier0_iface.h"
#include "tier0/dbg.h"

#endif // SHARED_PCH_H
