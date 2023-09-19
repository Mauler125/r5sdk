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
	const static int index = 10;
	CallVFunc<void>(index, this, bEnabled);
}

//-----------------------------------------------------------------------------
// Enables/disables the inputsystem windows message pump
//-----------------------------------------------------------------------------
void CInputSystem::EnableMessagePump(bool bEnabled)
{
	const static int index = 11;
	CallVFunc<void>(index, this, bEnabled);
}

//-----------------------------------------------------------------------------
// Poll current state
//-----------------------------------------------------------------------------
bool CInputSystem::IsButtonDown(ButtonCode_t Button)
{
	const static int index = 13;
	return CallVFunc<bool>(index, this, Button);
}

//-----------------------------------------------------------------------------
// Convert back + forth between ButtonCode/AnalogCode + strings
//-----------------------------------------------------------------------------
bool CInputSystem::ButtonCodeToString(ButtonCode_t Button)
{
	const static int index = 25;
	return CallVFunc<bool>(index, this, Button);
}
ButtonCode_t CInputSystem::StringToButtonCode(const char* pString)
{
	const static int index = 26;
	return CallVFunc<ButtonCode_t>(index, this, pString);
}

///////////////////////////////////////////////////////////////////////////////
CInputSystem* g_pInputSystem = nullptr;