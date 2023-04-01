#include "core/stdafx.h"
#include "core/logdef.h"

std::shared_ptr<spdlog::logger> g_TermLogger;
std::shared_ptr<spdlog::logger> g_ImGuiLogger;

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

#ifndef NETCONSOLE
	g_svLogSessionDirectory = fmt::format("platform\\logs\\{:s}\\", CreateTimedFileName());
	/************************
	 * IMGUI LOGGER SETUP   *
	 ************************/
	{
		g_ImGuiLogger = std::make_shared<spdlog::logger>("game_console", g_LogSink);
		spdlog::register_logger(g_ImGuiLogger); // in-game console logger.
		g_ImGuiLogger->set_pattern("[0.000] %v");
		g_ImGuiLogger->set_level(spdlog::level::trace);
	}
#endif // !NETCONSOLE
	/************************
	 * WINDOWS LOGGER SETUP *
	 ************************/
	{
#ifdef NETCONSOLE
		g_TermLogger = spdlog::default_logger();
#else
		g_TermLogger = spdlog::stdout_logger_mt("win_console");
#endif // NETCONSOLE

		// Determine if user wants ansi-color logging in the terminal.
		if (g_svCmdLine.find("-ansicolor") != string::npos)
		{
			g_TermLogger->set_pattern("[0.000] %v\u001b[0m");
			g_bSpdLog_UseAnsiClr = true;
		}
		else
		{
			g_TermLogger->set_pattern("[0.000] %v");
		}
		//g_TermLogger->set_level(spdlog::level::trace);
	}

#ifndef NETCONSOLE
	spdlog::set_default_logger(g_TermLogger); // Set as default.
	SpdLog_Create();
#endif // !NETCONSOLE

	spdlog::set_level(spdlog::level::trace);
	bInitialized = true;
}

void SpdLog_Create()
{
	/************************
	 * ROTATE LOGGER SETUP  *
	 ************************/
	spdlog::rotating_logger_mt<spdlog::synchronous_factory>("squirrel_re(warning)"
		, fmt::format("{:s}{:s}", g_svLogSessionDirectory, "script_warning.log"), SPDLOG_MAX_SIZE, SPDLOG_NUM_FILE)->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
	spdlog::rotating_logger_mt<spdlog::synchronous_factory>("squirrel_re"
		, fmt::format("{:s}{:s}", g_svLogSessionDirectory, "script.log"), SPDLOG_MAX_SIZE, SPDLOG_NUM_FILE)->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
	spdlog::rotating_logger_mt<spdlog::synchronous_factory>("sdk"
		, fmt::format("{:s}{:s}", g_svLogSessionDirectory, "message.log"), SPDLOG_MAX_SIZE, SPDLOG_NUM_FILE)->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
	spdlog::rotating_logger_mt<spdlog::synchronous_factory>("sdk(warning)"
		, fmt::format("{:s}{:s}", g_svLogSessionDirectory, "warning.log"), SPDLOG_MAX_SIZE, SPDLOG_NUM_FILE)->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
	spdlog::rotating_logger_mt<spdlog::synchronous_factory>("sdk(error)"
		, fmt::format("{:s}{:s}", g_svLogSessionDirectory, "error.log"), SPDLOG_MAX_SIZE, SPDLOG_NUM_FILE)->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
	spdlog::rotating_logger_mt<spdlog::synchronous_factory>("net_trace"
		, fmt::format("{:s}{:s}", g_svLogSessionDirectory, "net_trace.log"), SPDLOG_MAX_SIZE, SPDLOG_NUM_FILE)->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
#ifndef DEDICATED
	spdlog::rotating_logger_mt<spdlog::synchronous_factory>("netconsole"
		, fmt::format("{:s}{:s}", g_svLogSessionDirectory, "netconsole.log"), SPDLOG_MAX_SIZE, SPDLOG_NUM_FILE)->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
#endif // !DEDICATED
	spdlog::rotating_logger_mt<spdlog::synchronous_factory>("filesystem"
		, fmt::format("{:s}{:s}", g_svLogSessionDirectory, "filesystem.log"), SPDLOG_MAX_SIZE, SPDLOG_NUM_FILE)->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
}

//#############################################################################
// SPDLOG POST INIT
//#############################################################################
void SpdLog_PostInit()
{
#ifndef NETCONSOLE
	spdlog::flush_every(std::chrono::seconds(5)); // Flush buffers every 5 seconds for every logger.
	g_ImGuiLogger->set_pattern("%v");
#endif // !NETCONSOLE

	if (g_svCmdLine.find("-ansicolor") != string::npos)
	{
		g_TermLogger->set_pattern("%v\u001b[0m");
		g_bSpdLog_UseAnsiClr = true;
	}
	else { g_TermLogger->set_pattern("%v"); }
	g_bSpdLog_PostInit = true;
}

//#############################################################################
// SPDLOG SHUTDOWN
//#############################################################################
void SpdLog_Shutdown()
{
	spdlog::shutdown();
}
