//=====================================================================================//
//
// Purpose: Master for refresh, status bar, console, chat, notify, etc.
//
// $NoKeywords: $
//=====================================================================================//

#include <core/stdafx.h>
#include <engine/sys_dll.h>
#include <engine/gl_screen.h>
#include <vgui/vgui_baseui_interface.h>

//-----------------------------------------------------------------------------
// Purpose: finished loading
//-----------------------------------------------------------------------------
void SCR_EndLoadingPlaque(void)
{
	if (*scr_drawloading)
	{
		*scr_engineevent_loadingstarted = 0;
		g_pEngineVGui->HideLoadingPlaque();
	}
	else if (*gfExtendedError)
	{
		g_pEngineVGui->ShowErrorMessage();
	}
}
