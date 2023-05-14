#pragma once

#include <sstream>
#include "thirdparty/spdlog/spdlog.h"
#include "thirdparty/spdlog/async.h"
#include "thirdparty/spdlog/sinks/ostream_sink.h"
#include "thirdparty/spdlog/sinks/basic_file_sink.h"
#include "thirdparty/spdlog/sinks/stdout_sinks.h"
#include "thirdparty/spdlog/sinks/stdout_color_sinks.h"
#include "thirdparty/spdlog/sinks/ansicolor_sink.h"
#include "thirdparty/spdlog/sinks/rotating_file_sink.h"

constexpr int SPDLOG_MAX_SIZE = 10 * 1024 * 1024; // Sets number of bytes before rotating logger.
constexpr int SPDLOG_NUM_FILE = 512; // Sets number of files to rotate to.

inline bool g_bSpdLog_UseAnsiClr = false;
inline bool g_bSpdLog_PostInit = false;

extern std::shared_ptr<spdlog::logger> g_TermLogger;
extern std::shared_ptr<spdlog::logger> g_ImGuiLogger;

//-------------------------------------------------------------------------
// IMGUI CONSOLE SINK                                                     |
inline std::ostringstream g_LogStream;
inline auto g_LogSink = std::make_shared<spdlog::sinks::ostream_sink_st>(g_LogStream);

void SpdLog_Init(void);
void SpdLog_Create(void);
void SpdLog_PostInit(void);
void SpdLog_Shutdown(void);
