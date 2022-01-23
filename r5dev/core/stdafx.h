#pragma once
#pragma message("Pre-compiling headers.\n")

#define WIN32_LEAN_AND_MEAN // Prevent winsock2 redefinition.
#include <windows.h>
#include <WinSock2.h>

#include <stdio.h>
#include <Psapi.h>
#include <shlobj.h>
#include <objbase.h>
#include <emmintrin.h>
#include <cmath>
#include <vector>
#include <thread>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <filesystem>

#if !defined(DEDICATED)
#include <d3d11.h>
#endif // !DEDICATED

#include "thirdparty/detours/include/detours.h"
#include "thirdparty/detours/include/idetour.h"

#if !defined(DEDICATED)
#include "thirdparty/imgui/include/imgui.h"
#include "thirdparty/imgui/include/imgui_stdlib.h"
#include "thirdparty/imgui/include/imgui_utility.h"
#include "thirdparty/imgui/include/imgui_internal.h"
#include "thirdparty/imgui/include/imgui_impl_dx11.h"
#include "thirdparty/imgui/include/imgui_impl_win32.h"
#endif // !DEDICATED

#if !defined(SDKLAUNCHER)
#include "thirdparty/lzham/include/lzham_types.h"
#include "thirdparty/lzham/include/lzham.h"
#endif // !SDKLAUNCHER

#include "thirdparty/spdlog/include/spdlog.h"
#include "thirdparty/spdlog/include/async.h"
#include "thirdparty/spdlog/include/sinks/ostream_sink.h"
#include "thirdparty/spdlog/include/sinks/basic_file_sink.h"
#include "thirdparty/spdlog/include/sinks/stdout_sinks.h"
#include "thirdparty/spdlog/include/sinks/stdout_color_sinks.h"
#include "thirdparty/spdlog/include/sinks/ansicolor_sink.h"
#include "thirdparty/spdlog/include/sinks/rotating_file_sink.h"

#include "public/include/utility.h"
#include "public/include/memaddr.h"
#include "public/include/httplib.h"
#include "public/include/json.hpp"

#include "core/assert.h"
#include "core/termutil.h"
#include "tier0/basetypes.h"

#if !defined (SDKLAUNCHER)
namespace
{
#if !defined (DEDICATED)
	MODULE g_mGameDll = MODULE("r5apex.exe");
#else
	MODULE g_mGameDll = MODULE("r5apex_ds.exe");
#endif // !DEDICATED
	MODULE g_mRadVideoToolsDll   = MODULE("bink2w64.dll");
	MODULE g_mRadAudioDecoderDll = MODULE("binkawin64.dll");
	MODULE g_mRadAudioSystemDll  = MODULE("mileswin64.dll");
}
#endif // !SDKLAUNCHER

// Since we wanna be able to use it anywhere I thought this might be the best location for it. Since it gets inlined anyway.
#define MEMBER_AT_OFFSET(varType, varName, offset) \
	varType& varName() \
    { \
		static int _##varName = offset; \
		return *(varType*)((std::uintptr_t)this + _##varName); \
	}

template <typename ReturnType, typename ...Args>
ReturnType CallVFunc(int index, void* thisPtr, Args... args)
{
	return (*reinterpret_cast<ReturnType(__fastcall***)(void*, Args...)>(thisPtr))[index](thisPtr, args...);
}