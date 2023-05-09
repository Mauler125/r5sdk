#pragma once
#include "shared_pch.h"

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
#include "thirdparty/imgui/imgui.h"
#include "thirdparty/imgui/imgui_internal.h"
#include "thirdparty/imgui/misc/imgui_logger.h"
#include "thirdparty/imgui/misc/imgui_editor.h"
#include "thirdparty/imgui/misc/imgui_utility.h"
#include "thirdparty/imgui/misc/cpp/imgui_stdlib.h"
#include "thirdparty/imgui/backends/imgui_impl_dx11.h"
#include "thirdparty/imgui/backends/imgui_impl_win32.h"
#endif // !DEDICATED && !SDKLAUNCHER && !NETCONSOLE && !PLUGINSDK

#if !defined(SDKLAUNCHER) && !defined (NETCONSOLE) && !defined(PLUGINSDK)
#include "thirdparty/lzham/include/lzham_assert.h"
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

#include "tier1/cvar.h"
#include "tier1/cmd.h"
#include "common/global.h"
