#include "core/stdafx.h"
#include "core/logdef.h"
#include "vphysics/QHull.h"
#ifndef DEDICATED
#include "gameui/IConsole.h"
#endif // !DEDICATED

//-----------------------------------------------------------------------------
// Purpose: qhull error and debug prints
//-----------------------------------------------------------------------------
int HQHull_PrintFunc(const char* fmt, ...)
{
	static char buf[1024] = {};

	static std::shared_ptr<spdlog::logger> iconsole = spdlog::get("game_console");
	static std::shared_ptr<spdlog::logger> wconsole = spdlog::get("win_console");
	static std::shared_ptr<spdlog::logger> qhlogger = spdlog::get("qhull_debug_logger");

	{/////////////////////////////
		va_list args{};
		va_start(args, fmt);

		vsnprintf(buf, sizeof(buf), fmt, args);

		buf[sizeof(buf) - 1] = 0;
		va_end(args);
	}/////////////////////////////

	qhlogger->debug(buf);
	wconsole->debug(buf);

#ifndef DEDICATED
	g_spd_sys_w_oss.str("");
	g_spd_sys_w_oss.clear();

	iconsole->debug(buf);

	std::string s = g_spd_sys_w_oss.str();
	g_pIConsole->m_ivConLog.push_back(Strdup(s.c_str()));
#endif // !DEDICATED

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
