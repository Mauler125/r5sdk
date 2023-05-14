#pragma once
#include "shared_pch.h"
#if !defined(DEDICATED) && !defined (NETCONSOLE) && !defined(PLUGINSDK)
#include <d3d11.h>
#endif // !DEDICATED && !NETCONSOLE && !PLUGINSDK

// Thirdparty includes.
#include "thirdparty/detours/include/detours.h"
#include "thirdparty/detours/include/idetour.h"

#include "thirdparty/lzham/include/lzham_assert.h"
#include "thirdparty/lzham/include/lzham_types.h"
#include "thirdparty/lzham/include/lzham.h"

#include "thirdparty/curl/include/curl/curl.h"
#include "thirdparty/nlohmann/json.hpp"

#if !defined(DEDICATED) && !defined (NETCONSOLE) && !defined(PLUGINSDK)
#include "thirdparty/imgui/imgui.h"
#include "thirdparty/imgui/imgui_internal.h"
#include "thirdparty/imgui/misc/imgui_logger.h"
#include "thirdparty/imgui/misc/imgui_editor.h"
#include "thirdparty/imgui/misc/imgui_utility.h"
#include "thirdparty/imgui/misc/cpp/imgui_stdlib.h"
#include "thirdparty/imgui/backends/imgui_impl_dx11.h"
#include "thirdparty/imgui/backends/imgui_impl_win32.h"
#endif // !DEDICATED && !NETCONSOLE && !PLUGINSDK

#include "thirdparty/spdlog/spdlog.h"
#include "thirdparty/spdlog/async.h"
#include "thirdparty/spdlog/sinks/ostream_sink.h"
#include "thirdparty/spdlog/sinks/basic_file_sink.h"
#include "thirdparty/spdlog/sinks/stdout_sinks.h"
#include "thirdparty/spdlog/sinks/stdout_color_sinks.h"
#include "thirdparty/spdlog/sinks/ansicolor_sink.h"
#include "thirdparty/spdlog/sinks/rotating_file_sink.h"

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

// Tier1 includes.
#include "tier1/cvar.h"
#include "tier1/cmd.h"
#include "common/global.h"
