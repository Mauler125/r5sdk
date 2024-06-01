//=============================================================================//
//
// Purpose: General system utilities.
//
//=============================================================================//

#include "core/stdafx.h"
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
void _Error(const char* fmt, ...)
{
	char buf[4096];
	bool shouldNewline = true;
	{/////////////////////////////
		va_list args;
		va_start(args, fmt);

		int len = vsnprintf(buf, sizeof(buf), fmt, args);

		buf[sizeof(buf) - 1] = '\0';
		va_end(args);

		if (len > 0)
			shouldNewline = buf[len-1] != '\n';
	}/////////////////////////////

	Error(eDLL_T::ENGINE, NO_ERROR, shouldNewline ? "%s\n" : "%s", buf);
	v_Error("%s", buf);
}

//-----------------------------------------------------------------------------
// Purpose: Show warning in the console, exit engine with error when level 5
// Input  : level -
//			*error - ... - 
// Output : void* _Warning
//-----------------------------------------------------------------------------
void _Warning(int level, const char* fmt, ...)
{
	char buf[10000];
	bool shouldNewline = true;
	{/////////////////////////////
		va_list args;
		va_start(args, fmt);

		int len = vsnprintf(buf, sizeof(buf), fmt, args);

		buf[sizeof(buf) - 1] = '\0';
		va_end(args);

		if (len > 0)
			shouldNewline = buf[len - 1] != '\n';
	}/////////////////////////////

	if (level < 5)
	{
		Warning(eDLL_T::ENGINE, shouldNewline ? "Warning(%d):%s\n" : "Warning(%d):%s", level, buf);
	}

	v_Warning(level, "%s", buf);
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
		va_list args;
		va_start(args, fmt);

		vsnprintf(buf, sizeof(buf), fmt, args);

		buf[sizeof(buf) - 1] = '\0';
		va_end(args);
	}/////////////////////////////

	g_TextOverlay.m_nCon_NPrintf_Idx = pos;
	snprintf(g_TextOverlay.m_szCon_NPrintf_Buf,
		sizeof(g_TextOverlay.m_szCon_NPrintf_Buf), "%s", buf);
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

//-----------------------------------------------------------------------------
// Purpose: Gets the build string of the game (defined in build.txt), if the file
// is absent, the changelist # will be returned instead
//-----------------------------------------------------------------------------
const char* Sys_GetBuildString(void)
{
	return v_Sys_GetBuildString();
}

//-----------------------------------------------------------------------------
// Purpose: Gets the platform string
//-----------------------------------------------------------------------------
const char* Sys_GetPlatformString(void)
{
	return "PC";
}

void VSys_Utils::Detour(const bool bAttach) const
{
	DetourSetup(&v_Error, &_Error, bAttach);
	DetourSetup(&v_Warning, &_Warning, bAttach);
#ifndef DEDICATED
	DetourSetup(&v_Con_NPrintf, &_Con_NPrintf, bAttach);
#endif // !DEDICATED
}
