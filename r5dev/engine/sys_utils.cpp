//=============================================================================//
//
// Purpose: General system utilities.
//
//=============================================================================//

#include "core/stdafx.h"
#include "core/logdef.h"
#include "tier0/commandline.h"
#include "tier1/cvar.h"
#include "engine/sys_utils.h"
#ifdef DEDICATED
#include "engine/server/sv_rcon.h"
#else
#include "vgui/vgui_debugpanel.h"
#endif // !DEDICATED


//-----------------------------------------------------------------------------
// Purpose: Exit engine with error
// Input  : *error - 
//			... - 
// Output : void Sys_Error
//-----------------------------------------------------------------------------
void HSys_Error(char* fmt, ...)
{
	static char buf[1024] = {};

	va_list args{};
	va_start(args, fmt);

	vsnprintf(buf, sizeof(buf), fmt, args);

	buf[sizeof(buf) -1] = 0;
	va_end(args);

	Error(eDLL_T::ENGINE, "%s", buf);
	return v_Sys_Error(buf);
}

//-----------------------------------------------------------------------------
// Purpose: Show warning in the console, exit engine with error when level 5
// Input  : level -
//			*error - ... - 
// Output : void* Sys_Warning
//-----------------------------------------------------------------------------
void* HSys_Warning(int level, char* fmt, ...)
{
	static char buf[1024] = {};
	{/////////////////////////////
		va_list args{};
		va_start(args, fmt);

		vsnprintf(buf, sizeof(buf), fmt, args);

		buf[sizeof(buf) - 1] = '\0';
		va_end(args);
	}/////////////////////////////

	Warning(eDLL_T::COMMON, "Warning(%d):%s", level, buf);
	return v_Sys_Warning(level, buf);
}

#ifndef DEDICATED
//-----------------------------------------------------------------------------
// Purpose: Builds log to be displayed on the screen
// Input  : pos - 
//			*fmt - ... - 
// Output : void NPrintf
//-----------------------------------------------------------------------------
void HCon_NPrintf(int pos, const char* fmt, ...)
{
	if (cl_showhoststats->GetBool())
	{
		static char buf[1024] = {};
		{/////////////////////////////
			va_list args{};
			va_start(args, fmt);

			vsnprintf(buf, sizeof(buf), fmt, args);

			buf[sizeof(buf) - 1] = '\0';
			va_end(args);
		}/////////////////////////////

		snprintf(g_pLogSystem.m_pszCon_NPrintf_Buf, 
			sizeof(g_pLogSystem.m_pszCon_NPrintf_Buf), buf);
	}
}
#endif // !DEDICATED

//-----------------------------------------------------------------------------
// Purpose: Gets the process up time (input buffer should be at least 4096 bytes in size)
// Input  : *szBuffer - 
// Output : snprintf_s ret val
//-----------------------------------------------------------------------------
int Sys_GetProcessUpTime(char* szBuffer)
{
	return v_Sys_GetProcessUpTime(szBuffer);
}

void SysUtils_Attach()
{
	//DetourAttach((LPVOID*)&Sys_Error, &HSys_Error);
	DetourAttach((LPVOID*)&v_Sys_Warning, &HSys_Warning);
#ifndef DEDICATED
	DetourAttach((LPVOID*)&v_Con_NPrintf, &HCon_NPrintf);
#endif // !DEDICATED
}

void SysUtils_Detach()
{
	//DetourDetach((LPVOID*)&Sys_Error, &HSys_Error);
	DetourDetach((LPVOID*)&v_Sys_Warning, &HSys_Warning);
#ifndef DEDICATED
	DetourDetach((LPVOID*)&v_Con_NPrintf, &HCon_NPrintf);
#endif // !DEDICATED
}
