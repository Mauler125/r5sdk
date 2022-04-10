//===========================================================================//
//
// Purpose: Implements all the functions exported by the GameUI dll.
//
// $NoKeywords: $
//===========================================================================//

#include <core/stdafx.h>
#include <tier1/cvar.h>
#include <engine/sys_utils.h>
#include <vgui/vgui_debugpanel.h>
#include <vgui/vgui_baseui_interface.h>

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int HCEngineVGui_Paint(void* thisptr, PaintMode_t mode)
{
	int result = CEngineVGui_Paint(thisptr, mode);

	static void* pCMatSystemSurface = CMemory(0x14D40B3B0).RCast<void* (*)()>();
	static auto fnRenderStart = CMemory(0x14053EFC0).RCast<void(*)(void*)>();
	static auto fnRenderEnd = CMemory(0x14053F1B0).RCast<void* (*)()>();

	if (mode == PaintMode_t::PAINT_UIPANELS || mode == PaintMode_t::PAINT_INGAMEPANELS) // Render in-main menu and in-game.
	{
		fnRenderStart(pCMatSystemSurface);

		g_pLogSystem.Update();

		fnRenderEnd();
	}

	return result;
}

///////////////////////////////////////////////////////////////////////////////
void CEngineVGui_Attach()
{
	//DetourAttach((LPVOID*)&CEngineVGui_Paint, &HCEngineVGui_Paint);
}

void CEngineVGui_Detach()
{
	//DetourDetach((LPVOID*)&CEngineVGui_Paint, &HCEngineVGui_Paint);
}

///////////////////////////////////////////////////////////////////////////////