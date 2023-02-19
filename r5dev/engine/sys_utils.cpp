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

#if !defined( _X360 )
#define	MAXPRINTMSG	4096
#else
#define	MAXPRINTMSG	1024
#endif

//-----------------------------------------------------------------------------
// Purpose: Show error in the console
// Input  : *error - 
//			... - 
// Output : void _Error
//-----------------------------------------------------------------------------
void _Error(char* fmt, ...)
{
	char buf[4096];

	va_list args{};
	va_start(args, fmt);

	vsnprintf(buf, sizeof(buf), fmt, args);

	buf[sizeof(buf) -1] = 0;
	va_end(args);

	Error(eDLL_T::ENGINE, NO_ERROR, "%s", buf);
	v_Error(buf);
}

//-----------------------------------------------------------------------------
// Purpose: Show warning in the console, exit engine with error when level 5
// Input  : level -
//			*error - ... - 
// Output : void* _Warning
//-----------------------------------------------------------------------------
void* _Warning(int level, char* fmt, ...)
{
	char buf[10000];
	{/////////////////////////////
		va_list args{};
		va_start(args, fmt);

		vsnprintf(buf, sizeof(buf), fmt, args);

		buf[sizeof(buf) - 1] = '\0';
		va_end(args);
	}/////////////////////////////

	if (level < 5)
	{
		Warning(eDLL_T::COMMON, "Warning(%d):%s", level, buf);
	}

	v_Warning(level, buf);
}

#ifndef DEDICATED
//-----------------------------------------------------------------------------
// Purpose: Builds log to be displayed on the screen
// Input  : pos - 
//			*fmt - ... - 
// Output : void NPrintf
//-----------------------------------------------------------------------------
void _Con_NPrintf(int pos, const char* fmt, ...)
{
	char buf[MAXPRINTMSG];
	{/////////////////////////////
		va_list args{};
		va_start(args, fmt);

		vsnprintf(buf, sizeof(buf), fmt, args);

		buf[sizeof(buf) - 1] = '\0';
		va_end(args);
	}/////////////////////////////

	g_pOverlay->m_nCon_NPrintf_Idx = pos;
	snprintf(g_pOverlay->m_szCon_NPrintf_Buf,
		sizeof(g_pOverlay->m_szCon_NPrintf_Buf), buf);
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

void VSys_Utils::Attach() const
{
	DetourAttach((LPVOID*)&v_Error, &_Error);
	DetourAttach((LPVOID*)&v_Warning, &_Warning);
#ifndef DEDICATED
	DetourAttach((LPVOID*)&v_Con_NPrintf, &_Con_NPrintf);
#endif // !DEDICATED
}

void VSys_Utils::Detach() const
{
	DetourDetach((LPVOID*)&v_Error, &_Error);
	DetourDetach((LPVOID*)&v_Warning, &_Warning);
#ifndef DEDICATED
	DetourDetach((LPVOID*)&v_Con_NPrintf, &_Con_NPrintf);
#endif // !DEDICATED
}
