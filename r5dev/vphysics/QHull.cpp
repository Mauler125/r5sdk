#include "core/stdafx.h"
#include "core/logdef.h"
#include "vphysics/QHull.h"

//-----------------------------------------------------------------------------
// Purpose: qhull error and debug prints
//-----------------------------------------------------------------------------
int HQHull_PrintFunc(const char* fmt, ...)
{
	static bool initialized = false;
	static char buf[1024];

	static auto iconsole = spdlog::stdout_logger_mt("qhull_print_iconsole"); // in-game console.
	static auto wconsole = spdlog::stdout_logger_mt("qhull_print_wconsole"); // windows console.
	static auto qhlogger = spdlog::basic_logger_mt("qhull_print_logger", "platform\\logs\\qhull_print.log"); // file logger.

	g_spd_sqvm_p_oss.str("");
	g_spd_sqvm_p_oss.clear();

	if (!initialized)
	{
		iconsole = std::make_shared<spdlog::logger>("qhull_print_ostream", g_spd_sqvm_p_ostream_sink);
		iconsole->set_pattern("[%S.%e] %v");
		iconsole->set_level(spdlog::level::debug);
		wconsole->set_pattern("[%S.%e] %v");
		wconsole->set_level(spdlog::level::debug);
		qhlogger->set_pattern("[%S.%e] %v");
		qhlogger->set_level(spdlog::level::debug);
		initialized = true;
	}

	va_list args;
	va_start(args, fmt);

	vsnprintf(buf, sizeof(buf), fmt, args);

	buf[sizeof(buf) - 1] = 0;
	va_end(args);

	qhlogger->debug(buf);
	iconsole->debug(buf);
	wconsole->debug(buf);

	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
void QHull_Attach()
{
	DetourAttach((LPVOID*)&QHull_PrintFunc, &HQHull_PrintFunc);
}

void QHull_Detach()
{
	DetourDetach((LPVOID*)&QHull_PrintFunc, &HQHull_PrintFunc);
}
