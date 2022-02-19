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
		using HideLoadingPlaqueFn = void(*)(void*);
		(*reinterpret_cast<HideLoadingPlaqueFn**>(g_pEngineVGui))[36](g_pEngineVGui); // (*((void(__fastcall**)(void**))g_CEngineVGui + 36))(&g_CEngineVGui);// HideLoadingPlaque
	}
	else if (*gfExtendedError)
	{
		using ShowErrorMessageFn = void(*)(void*);
		(*reinterpret_cast<ShowErrorMessageFn**>(g_pEngineVGui))[35](g_pEngineVGui); // (*((void(__fastcall**)(void**))g_CEngineVGui + 35))(&g_CEngineVGui);// ShowErrorMessage
	}
}
