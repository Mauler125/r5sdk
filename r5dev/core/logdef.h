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

extern std::shared_ptr<spdlog::logger> g_TermLogger;
extern std::shared_ptr<spdlog::logger> g_ImGuiLogger;

//-------------------------------------------------------------------------
// IMGUI CONSOLE SINK                                                     |
extern std::ostringstream g_LogStream;
extern std::shared_ptr<spdlog::sinks::ostream_sink_st> g_LogSink;

void SpdLog_Init(const bool bAnsiColor);
void SpdLog_Create(void);
void SpdLog_Shutdown(void);
