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
	static char buf[4096] = {};

	static std::shared_ptr<spdlog::logger> iconsole = spdlog::get("game_console");
	static std::shared_ptr<spdlog::logger> wconsole = spdlog::get("win_console");
	static std::shared_ptr<spdlog::logger> qhlogger = spdlog::get("qhull_info");

	g_LogMutex.lock();
	{/////////////////////////////
		va_list args{};
		va_start(args, fmt);

		vsnprintf(buf, sizeof(buf), fmt, args);

		buf[sizeof(buf) - 1] = '\0';
		va_end(args);
	}/////////////////////////////

	qhlogger->debug(buf);
	wconsole->debug(buf);

#ifndef DEDICATED
	iconsole->debug(buf);
	g_pConsole->AddLog(ConLog_t(g_LogStream.str(), ImVec4(0.81f, 0.81f, 0.81f, 1.00f)));

	g_LogStream.str("");
	g_LogStream.clear();
#endif // !DEDICATED

	g_LogMutex.unlock();
	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
void VQHull::Attach() const
{
	DetourAttach((LPVOID*)&QHull_PrintFunc, &HQHull_PrintFunc);
}

void VQHull::Detach() const
{
	DetourDetach((LPVOID*)&QHull_PrintFunc, &HQHull_PrintFunc);
}
