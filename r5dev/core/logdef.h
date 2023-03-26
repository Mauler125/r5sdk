#pragma once

constexpr int SPDLOG_MAX_SIZE = 10 * 1024 * 1024; // Sets number of bytes before rotating logger.
constexpr int SPDLOG_NUM_FILE = 512; // Sets number of files to rotate to.

inline bool g_bSpdLog_UseAnsiClr = false;
inline bool g_bSpdLog_PostInit = false;

inline string g_svLogSessionDirectory;

//-------------------------------------------------------------------------
// IMGUI CONSOLE SINK                                                     |
inline std::ostringstream g_LogStream;
inline auto g_LogSink = std::make_shared<spdlog::sinks::ostream_sink_st>(g_LogStream);

void SpdLog_Init(void);
void SpdLog_PostInit(void);
void SpdLog_Shutdown(void);
