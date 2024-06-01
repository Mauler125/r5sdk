#include "core/stdafx.h"
#include "vphysics/QHull.h"
#ifndef DEDICATED
#include "gameui/IConsole.h"
#endif // !DEDICATED

//-----------------------------------------------------------------------------
// Purpose: qhull error and debug prints
//-----------------------------------------------------------------------------
int QHull_PrintFunc(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	CoreMsgV(LogType_t::LOG_INFO, LogLevel_t::LEVEL_NOTIFY, eDLL_T::COMMON, "message", fmt, args);
	va_end(args);

	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
void VQHull::Detour(const bool bAttach) const
{
	DetourSetup(&v_QHull_PrintFunc, &QHull_PrintFunc, bAttach);
}

