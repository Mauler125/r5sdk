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

////-----------------------------------------------------------------------------
//// Enables/disables input
////-----------------------------------------------------------------------------
//void CInputSystem::EnableInput(bool bEnabled)
//{
//	const static int index = 10;
//	CallVFunc<void>(index, this, bEnabled);
//}
//
////-----------------------------------------------------------------------------
//// Enables/disables the inputsystem windows message pump
////-----------------------------------------------------------------------------
//void CInputSystem::EnableMessagePump(bool bEnabled)
//{
//	const static int index = 11;
//	CallVFunc<void>(index, this, bEnabled);
//}
//
////-----------------------------------------------------------------------------
//// Poll current state
////-----------------------------------------------------------------------------
//bool CInputSystem::IsButtonDown(ButtonCode_t Button)
//{
//	const static int index = 13;
//	return CallVFunc<bool>(index, this, Button);
//}
//
////-----------------------------------------------------------------------------
//// Convert back + forth between ButtonCode/AnalogCode + strings
////-----------------------------------------------------------------------------
//bool CInputSystem::ButtonCodeToString(ButtonCode_t Button)
//{
//	const static int index = 25;
//	return CallVFunc<bool>(index, this, Button);
//}
//ButtonCode_t CInputSystem::StringToButtonCode(const char* pString)
//{
//	const static int index = 26;
//	return CallVFunc<ButtonCode_t>(index, this, pString);
//}

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
