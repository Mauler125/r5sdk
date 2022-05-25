#pragma once
#pragma message("Pre-compiling headers.\n")

#define WIN32_LEAN_AND_MEAN // Prevent winsock2 redefinition.
#include <windows.h>
#include <WinSock2.h>
#include <comdef.h>
#include <gdiplus.h>
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

#if !defined(DEDICATED) && !defined(SDKLAUNCHER) && !defined (NETCONSOLE)
#include <d3d11.h>
#endif // !DEDICATED && !SDKLAUNCHER && !NETCONSOLE

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
#endif // SDKLAUNCHER

#if !defined(DEDICATED) && !defined(SDKLAUNCHER) && !defined (NETCONSOLE)
#include "thirdparty/imgui/include/imgui.h"
#include "thirdparty/imgui/include/imgui_stdlib.h"
#include "thirdparty/imgui/include/imgui_utility.h"
#include "thirdparty/imgui/include/imgui_internal.h"
#include "thirdparty/imgui/include/imgui_impl_dx11.h"
#include "thirdparty/imgui/include/imgui_impl_win32.h"
#endif // !DEDICATED && !SDKLAUNCHER && !NETCONSOLE

#if !defined(SDKLAUNCHER) && !defined (NETCONSOLE)
#include "thirdparty/lzham/include/lzham_types.h"
#include "thirdparty/lzham/include/lzham.h"
#endif // !SDKLAUNCHER && !NETCONSOLE

#include "thirdparty/spdlog/include/spdlog.h"
#include "thirdparty/spdlog/include/async.h"
#include "thirdparty/spdlog/include/sinks/ostream_sink.h"
#include "thirdparty/spdlog/include/sinks/basic_file_sink.h"
#include "thirdparty/spdlog/include/sinks/stdout_sinks.h"
#include "thirdparty/spdlog/include/sinks/stdout_color_sinks.h"
#include "thirdparty/spdlog/include/sinks/ansicolor_sink.h"
#include "thirdparty/spdlog/include/sinks/rotating_file_sink.h"

#include "common/pseudodefs.h"
#include "common/x86defs.h"
#include "common/sdkdefs.h"

#include "public/include/utility.h"
#include "public/include/memaddr.h"
#include "public/include/module.h"
#include "public/include/httplib.h"

#include "core/assert.h"
#include "core/termutil.h"
#include "tier0/basetypes.h"
#include "tier0/platform.h"
#include "tier0/dbg.h"

#if !defined(SDKLAUNCHER) && !defined (NETCONSOLE)
#if !defined (DEDICATED)
inline CModule g_mGameDll = CModule("r5apex.exe");
inline CModule g_mRadVideoToolsDll   = CModule("bink2w64.dll");
inline CModule g_mRadAudioDecoderDll = CModule("binkawin64.dll");
inline CModule g_mRadAudioSystemDll  = CModule("mileswin64.dll");
#else // No DirectX and Miles imports.
inline CModule g_mGameDll = CModule("r5apex_ds.exe");
#endif // !DEDICATED

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
#endif // !SDKLAUNCHER && !NETCONSOLE