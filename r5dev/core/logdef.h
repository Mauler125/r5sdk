#pragma once

//-------------------------------------------------------------------------
// NETCHAN                                                                |
inline auto g_spd_netchan_logger = spdlog::basic_logger_mt("netchan_logger", "platform\\logs\\net_trace.log");
inline std::ostringstream g_spd_net_p_oss;
inline auto g_spd_net_p_ostream_sink = std::make_shared<spdlog::sinks::ostream_sink_st>(g_spd_net_p_oss);
//-------------------------------------------------------------------------
// FILESYSTEM                                                             |
inline std::ostringstream fs_oss;
inline auto fs_ostream_sink = std::make_shared<spdlog::sinks::ostream_sink_st>(fs_oss);
//-------------------------------------------------------------------------
// SQUIRREL PRINTF                                                        |
inline std::ostringstream g_spd_sqvm_p_oss;
inline auto g_spd_sqvm_p_ostream_sink = std::make_shared<spdlog::sinks::ostream_sink_st>(g_spd_sqvm_p_oss);
//-------------------------------------------------------------------------
// SQUIRREL WARNF                                                         |
inline std::ostringstream g_spd_sqvm_w_oss;
inline auto g_spd_sqvm_w_ostream_sink = std::make_shared<spdlog::sinks::ostream_sink_st>(g_spd_sqvm_w_oss);
//-------------------------------------------------------------------------
// SYSTEM PRINTF                                                          |
inline std::ostringstream g_spd_sys_w_oss;
inline auto g_spd_sys_p_ostream_sink = std::make_shared<spdlog::sinks::ostream_sink_st>(g_spd_sys_w_oss);
//-------------------------------------------------------------------------
// QHULL PRINTF                                                           |
inline std::ostringstream g_spd_qhull_p_w_oss;
inline auto g_spd_qhull_p_ostream_sink = std::make_shared<spdlog::sinks::ostream_sink_st>(g_spd_qhull_p_w_oss);
