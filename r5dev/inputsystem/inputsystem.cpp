//===========================================================================//
//
// Purpose: 
//
//===========================================================================//

#include "core/stdafx.h"
#include "vpc/IAppSystem.h"
#include "inputsystem/inputsystem.h"

//-----------------------------------------------------------------------------
// Enables/disables input
//-----------------------------------------------------------------------------
void CInputSystem::EnableInput(bool bEnabled)
{
	const int index = 10;
	CallVFunc<void>(index, this, bEnabled);
}

//-----------------------------------------------------------------------------
// Enables/disables the inputsystem windows message pump
//-----------------------------------------------------------------------------
void CInputSystem::EnableMessagePump(bool bEnabled)
{
	const int index = 11;
	CallVFunc<void>(index, this, bEnabled);
}

//-----------------------------------------------------------------------------
// Poll current state
//-----------------------------------------------------------------------------
bool CInputSystem::IsButtonDown(ButtonCode_t Button)
{
	const int index = 13;
	return CallVFunc<bool>(index, this, Button);
}

///////////////////////////////////////////////////////////////////////////////
CInputSystem* g_pInputSystem = nullptr;