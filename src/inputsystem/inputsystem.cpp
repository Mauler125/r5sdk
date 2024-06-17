//===========================================================================//
//
// Purpose: 
//
//===========================================================================//

#include "core/stdafx.h"
#include "vpc/IAppSystem.h"
#include "windows/id3dx.h"
#include "geforce/reflex.h"
#include "inputsystem/inputsystem.h"
#include <materialsystem/cmaterialsystem.h>

//-----------------------------------------------------------------------------
// Returns the currently attached window
//-----------------------------------------------------------------------------
PlatWindow_t CInputSystem::GetAttachedWindow() const
{
	return (PlatWindow_t)m_hAttachedHWnd;
}

LRESULT CInputSystem::WindowProc(void* unused, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (g_pInputSystem->m_bEnabled &&
		((hwnd == g_pInputSystem->m_hAttachedHWnd) || (uMsg == WM_ACTIVATEAPP)) &&
		(uMsg != WM_CLOSE))
	{
		if (PCLSTATS_IS_PING_MSG_ID(uMsg))
		{
			GFX_SetLatencyMarker(D3D11Device(), PC_LATENCY_PING, MaterialSystem()->GetCurrentFrameCount());
		}
	}

	return CInputSystem__WindowProc(unused, hwnd, uMsg, wParam, lParam);
}


void VInputSystem::Detour(const bool bAttach) const
{
	DetourSetup(&CInputSystem__WindowProc, &CInputSystem::WindowProc, bAttach);
}

///////////////////////////////////////////////////////////////////////////////
CInputSystem* g_pInputSystem = nullptr;
bool(**g_fnSyncRTWithIn)(void) = nullptr;
