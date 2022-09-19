#pragma once

constexpr int SPDLOG_MAX_SIZE = 10 * 1024 * 1024; // Sets number of bytes before rotating logger.
constexpr int SPDLOG_NUM_FILE = 0; // Sets number of files to rotate to.

inline bool g_bSpdLog_UseAnsiClr = false;
inline bool g_bSpdLog_PostInit = false;

//-------------------------------------------------------------------------
// IMGUI CONSOLE SINK                                                     |
inline std::ostringstream g_spd_sys_w_oss;
inline auto g_spd_sys_p_ostream_sink = std::make_shared<spdlog::sinks::ostream_sink_st>(g_spd_sys_w_oss);

void SpdLog_Init(void);
void SpdLog_PostInit(void);
