#include "core/stdafx.h"
#include "core/logdef.h"

//#############################################################################
// SPDLOG INIT
//#############################################################################
void SpdLog_Init(void)
{
	static bool bInitialized = false;

	if (bInitialized)
	{
		Assert(bInitialized, "'SpdLog_Init()' has already been called.");
		return;
	}

	g_svLogSessionDirectory = fmt::format("platform\\logs\\{:s}\\", CreateTimedFileName());
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
		spdlog::rotating_logger_mt<spdlog::synchronous_factory>("sqvm_warn" , fmt::format("{:s}sqvm_warn.log"  , g_svLogSessionDirectory), SPDLOG_MAX_SIZE, SPDLOG_NUM_FILE)->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
		spdlog::rotating_logger_mt<spdlog::synchronous_factory>("sqvm_info" , fmt::format("{:s}sqvm_info.log"  , g_svLogSessionDirectory), SPDLOG_MAX_SIZE, SPDLOG_NUM_FILE)->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
		spdlog::rotating_logger_mt<spdlog::synchronous_factory>("sdk_info"  , fmt::format("{:s}sdk_info.log"   , g_svLogSessionDirectory), SPDLOG_MAX_SIZE, SPDLOG_NUM_FILE)->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
		spdlog::rotating_logger_mt<spdlog::synchronous_factory>("sdk_warn"  , fmt::format("{:s}sdk_warn.log"   , g_svLogSessionDirectory), SPDLOG_MAX_SIZE, SPDLOG_NUM_FILE)->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
		spdlog::rotating_logger_mt<spdlog::synchronous_factory>("sdk_error" , fmt::format("{:s}sdk_error.log"  , g_svLogSessionDirectory), SPDLOG_MAX_SIZE, SPDLOG_NUM_FILE)->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
		spdlog::rotating_logger_mt<spdlog::synchronous_factory>("qhull_info", fmt::format("{:s}qhull_info.log" , g_svLogSessionDirectory), SPDLOG_MAX_SIZE, SPDLOG_NUM_FILE)->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
		spdlog::rotating_logger_mt<spdlog::synchronous_factory>("net_trace" , fmt::format("{:s}net_trace.log"  , g_svLogSessionDirectory), SPDLOG_MAX_SIZE, SPDLOG_NUM_FILE)->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
#ifndef DEDICATED
		spdlog::rotating_logger_mt<spdlog::synchronous_factory>("net_con"   , fmt::format("{:s}net_console.log", g_svLogSessionDirectory), SPDLOG_MAX_SIZE, SPDLOG_NUM_FILE)->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
#endif // !DEDICATED
		spdlog::rotating_logger_mt<spdlog::synchronous_factory>("fs_warn"   , fmt::format("{:s}fs_warn.log"    ,g_svLogSessionDirectory), SPDLOG_MAX_SIZE, SPDLOG_NUM_FILE)->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
	}

	spdlog::set_level(spdlog::level::trace);
	bInitialized = true;
}

//#############################################################################
// SPDLOG POST INIT
//#############################################################################
void SpdLog_PostInit()
{
	spdlog::flush_every(std::chrono::seconds(5)); // Flush buffers every 5 seconds for every logger.

	std::shared_ptr<spdlog::logger> iconsole = spdlog::get("game_console");
	std::shared_ptr<spdlog::logger> wconsole = spdlog::get("win_console");

	iconsole->set_pattern("%v");

	if (g_svCmdLine.find("-ansiclr") != string::npos)
	{
		wconsole->set_pattern("%v\u001b[0m");
		g_bSpdLog_UseAnsiClr = true;
	}
	else { wconsole->set_pattern("%v"); }
	g_bSpdLog_PostInit = true;
}

//#############################################################################
// SPDLOG SHUTDOWN
//#############################################################################
void SpdLog_Shutdown()
{
	spdlog::shutdown();
}
