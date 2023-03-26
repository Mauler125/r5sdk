#include "core/stdafx.h"
#include "core/logdef.h"
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
	CoreMsgV(LogType_t::LOG_INFO, LogLevel_t::LEVEL_NOTIFY, eDLL_T::NONE, "message", fmt, args);
	va_end(args);

	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
void VQHull::Attach() const
{
	DetourAttach((LPVOID*)&v_QHull_PrintFunc, &QHull_PrintFunc);
}

void VQHull::Detach() const
{
	DetourDetach((LPVOID*)&v_QHull_PrintFunc, &QHull_PrintFunc);
}
