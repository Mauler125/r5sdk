#pragma once
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
#include <regex>
#include <thread>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <filesystem>
#include <set>
#include <unordered_set>

// Windows specifics.
#ifndef FILE_SUPPORTS_GHOSTING
#define FILE_SUPPORTS_GHOSTING 0x40000000  // winnt
#endif
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

#if !defined(DEDICATED) && !defined(SDKLAUNCHER) && !defined (NETCONSOLE) && !defined(PLUGINSDK)
#include <d3d11.h>
#endif // !DEDICATED && !SDKLAUNCHER && !NETCONSOLE && !PLUGINSDK

#include "thirdparty/nlohmann/json.hpp"

#include "thirdparty/detours/include/detours.h"
#include "thirdparty/detours/include/idetour.h"

#if defined(SDKLAUNCHER)
#include "thirdparty/cppnet/cppkore/Kore.h"
#include "thirdparty/cppnet/cppkore/UIXTheme.h"
#include "thirdparty/cppnet/cppkore/UIXLabel.h"
#include "thirdparty/cppnet/cppkore/UIXListView.h"
#include "thirdparty/cppnet/cppkore/UIXCheckBox.h"
#include "thirdparty/cppnet/cppkore/UIXComboBox.h"
#include "thirdparty/cppnet/cppkore/UIXTextBox.h"
#include "thirdparty/cppnet/cppkore/UIXGroupBox.h"
#include "thirdparty/cppnet/cppkore/UIXButton.h"
#include "thirdparty/cppnet/cppkore/UIXRadioButton.h"
#include "thirdparty/cppnet/cppkore/KoreTheme.h"

#include "launcher/launcherdefs.h"
#endif // SDKLAUNCHER

#if !defined(DEDICATED) && !defined(SDKLAUNCHER) && !defined (NETCONSOLE) && !defined(PLUGINSDK)
#include "thirdparty/imgui/include/imgui.h"
#include "thirdparty/imgui/include/imgui_stdlib.h"
#include "thirdparty/imgui/include/imgui_logger.h"
#include "thirdparty/imgui/include/imgui_editor.h"
#include "thirdparty/imgui/include/imgui_utility.h"
#include "thirdparty/imgui/include/imgui_internal.h"
#include "thirdparty/imgui/include/imgui_impl_dx11.h"
#include "thirdparty/imgui/include/imgui_impl_win32.h"
#endif // !DEDICATED && !SDKLAUNCHER && !NETCONSOLE && !PLUGINSDK

#if !defined(SDKLAUNCHER) && !defined (NETCONSOLE) && !defined(PLUGINSDK)
#include "thirdparty/lzham/include/lzham_types.h"
#include "thirdparty/lzham/include/lzham.h"
#endif // !SDKLAUNCHER && !NETCONSOLE && !PLUGINSDK

#include "thirdparty/spdlog/spdlog.h"
#include "thirdparty/spdlog/async.h"
#include "thirdparty/spdlog/sinks/ostream_sink.h"
#include "thirdparty/spdlog/sinks/basic_file_sink.h"
#include "thirdparty/spdlog/sinks/stdout_sinks.h"
#include "thirdparty/spdlog/sinks/stdout_color_sinks.h"
#include "thirdparty/spdlog/sinks/ansicolor_sink.h"
#include "thirdparty/spdlog/sinks/rotating_file_sink.h"

#include "thirdparty/curl/include/curl/curl.h"

#include "common/pseudodefs.h"
#include "common/x86defs.h"
#include "common/sdkdefs.h"

#include "public/utility/utility.h"
#include "public/utility/memaddr.h"
#include "public/utility/module.h"
#include "public/utility/sigcache.h"
#include "public/utility/vdf_parser.h"

#include "core/assert.h"
#include "core/termutil.h"
#include "tier0/basetypes.h"
#include "tier0/platform.h"
#include "tier0/annotations.h"
#include "tier0/commonmacros.h"
#if !defined(SDKLAUNCHER) && !defined(PLUGINSDK)
#include "tier0/dbg.h"
#endif // !SDKLAUNCHER && !NETCONSOLE && !PLUGINSDK

#if !defined(SDKLAUNCHER) && !defined (NETCONSOLE) && !defined(PLUGINSDK)
#if !defined (DEDICATED)
inline CModule g_GameDll = CModule("r5apex.exe");
inline CModule g_RadVideoToolsDll   = CModule("bink2w64.dll");
inline CModule g_RadAudioDecoderDll = CModule("binkawin64.dll");
inline CModule g_RadAudioSystemDll  = CModule("mileswin64.dll");
#if !defined (CLIENT_DLL)
inline CModule g_SDKDll = CModule("gamesdk.dll");
#else // This dll is loaded from 'bin/x64_retail//'
inline CModule g_SDKDll = CModule("client.dll");
#endif // !CLIENT_DLL
#else // No DirectX and Miles imports.
inline CModule g_GameDll = CModule("r5apex_ds.exe");
inline CModule g_SDKDll = CModule("dedicated.dll");
#endif // !DEDICATED
inline CSigCache g_SigCache;

#define VAR_NAME(varName)  #varName

#define MEMBER_AT_OFFSET(varType, varName, offset)             \
	varType& varName()                                         \
	{                                                          \
		static int _##varName = offset;                        \
		return *(varType*)((std::uintptr_t)this + _##varName); \
	}

template <typename ReturnType, typename ...Args>
ReturnType CallVFunc(int index, void* thisPtr, Args... args)
{
	return (*reinterpret_cast<ReturnType(__fastcall***)(void*, Args...)>(thisPtr))[index](thisPtr, args...);
}

inline void LogFunAdr(const char* szFun, uintptr_t nAdr) // Logging function addresses.
{
	spdlog::debug("| {:s}: {:42s}: {:#18x} |\n", "FUN", szFun, nAdr);
}
inline void LogVarAdr(const char* szVar, uintptr_t nAdr) // Logging variable addresses.
{
	spdlog::debug("| {:s}: {:42s}: {:#18x} |\n", "VAR", szVar, nAdr);
}
inline void LogConAdr(const char* szCon, uintptr_t nAdr) // Logging constant addresses.
{
	spdlog::debug("| {:s}: {:42s}: {:#18x} |\n", "CON", szCon, nAdr);
}
#endif // !SDKLAUNCHER && !NETCONSOLE && !PLUGINSDK