#include "core/stdafx.h"
#include "core/logdef.h"
#include "vphysics/QHull.h"
#include "engine/sys_utils.h"
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
	static std::shared_ptr<spdlog::logger> qhlogger = spdlog::get("qhull_info");

	s_LogMutex.lock();
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
	iconsole->debug(buf);
	g_pIConsole->m_ivConLog.push_back(CConLog(g_spd_sys_w_oss.str(), ImVec4(0.81f, 0.81f, 0.81f, 1.00f)));

	g_spd_sys_w_oss.str("");
	g_spd_sys_w_oss.clear();
#endif // !DEDICATED

	s_LogMutex.unlock();
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
