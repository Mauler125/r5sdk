#include "core/stdafx.h"
#include "core/logdef.h"

//#############################################################################
// SPDLOG SETUP
//#############################################################################
void SpdLog_Init(void)
{
	static bool bInitialized = false;

	if (bInitialized)
	{
		Assert(bInitialized, "'SpdLog_Init()' has already been called.");
		return;
	}

	/************************
	 * IMGUI LOGGER SETUP   *
	 ************************/
	{
		auto iconsole = std::make_shared<spdlog::logger>("game_console", g_spd_sys_p_ostream_sink);
		spdlog::register_logger(iconsole); // in-game console logger.
		iconsole->set_pattern("[0.000] %v");
		iconsole->set_level(spdlog::level::trace);
	}

	/************************
	 * WINDOWS LOGGER SETUP *
	 ************************/
	{
		auto wconsole = spdlog::stdout_logger_mt("win_console");

		// Determine if user wants ansi-color logging in the terminal.
		if (g_svCmdLine.find("-ansiclr") != string::npos)
		{
			wconsole->set_pattern("[0.000] %v\u001b[0m");
			g_bSpdLog_UseAnsiClr = true;
		}
		else { wconsole->set_pattern("[0.000] %v"); }
		wconsole->set_level(spdlog::level::trace);
		spdlog::set_default_logger(wconsole); // Set as default.
	}

	/************************
	 * ROTATE LOGGER SETUP  *
	 ************************/
	{
		spdlog::rotating_logger_mt<spdlog::synchronous_factory>("sqvm_warn" , "platform\\logs\\sqvm_warn.log"  , SPDLOG_MAX_SIZE, SPDLOG_NUM_FILE)->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
		spdlog::rotating_logger_mt<spdlog::synchronous_factory>("sqvm_info" , "platform\\logs\\sqvm_info.log"  , SPDLOG_MAX_SIZE, SPDLOG_NUM_FILE)->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
		spdlog::rotating_logger_mt<spdlog::synchronous_factory>("sdk_info"  , "platform\\logs\\sdk_info.log"   , SPDLOG_MAX_SIZE, SPDLOG_NUM_FILE)->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
		spdlog::rotating_logger_mt<spdlog::synchronous_factory>("sdk_warn"  , "platform\\logs\\sdk_warn.log"   , SPDLOG_MAX_SIZE, SPDLOG_NUM_FILE)->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
		spdlog::rotating_logger_mt<spdlog::synchronous_factory>("sdk_error" , "platform\\logs\\sdk_error.log"  , SPDLOG_MAX_SIZE, SPDLOG_NUM_FILE)->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
		spdlog::rotating_logger_mt<spdlog::synchronous_factory>("qhull_info", "platform\\logs\\qhull_info.log" , SPDLOG_MAX_SIZE, SPDLOG_NUM_FILE)->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
		spdlog::rotating_logger_mt<spdlog::synchronous_factory>("net_trace" , "platform\\logs\\net_trace.log"  , SPDLOG_MAX_SIZE, SPDLOG_NUM_FILE)->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
#ifndef DEDICATED
		spdlog::rotating_logger_mt<spdlog::synchronous_factory>("net_con"   , "platform\\logs\\net_console.log", SPDLOG_MAX_SIZE, SPDLOG_NUM_FILE)->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
#endif // !DEDICATED
		spdlog::rotating_logger_mt<spdlog::synchronous_factory>("fs_warn"   , "platform\\logs\\fs_warn.log"    , SPDLOG_MAX_SIZE, SPDLOG_NUM_FILE)->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
	}

	spdlog::set_level(spdlog::level::trace);
	spdlog::flush_every(std::chrono::seconds(5)); // Flush buffers every 5 seconds for every logger.

	bInitialized = true;
}

void SpdLog_PostInit()
{
	std::shared_ptr<spdlog::logger> iconsole = spdlog::get("game_console");
	std::shared_ptr<spdlog::logger> wconsole = spdlog::get("win_console");

	iconsole->set_pattern("%v");

	if (g_svCmdLine.find("-ansiclr") != string::npos)
	{
		wconsole->set_pattern("%v\u001b[0m");
		g_bSpdLog_UseAnsiClr = true;
	}
	else { wconsole->set_pattern("%v"); }
}