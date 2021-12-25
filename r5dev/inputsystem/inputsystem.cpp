#include "core/stdafx.h"
#include "vpc/IAppSystem.h"
#include "inputsystem/inputsystem.h"

///////////////////////////////////////////////////////////////////////////////
CInputSystem* g_pInputSystem = reinterpret_cast<CInputSystem*>(p_IAppSystem_LoadLibrary.FindPatternSelf("48 89 05", ADDRESS::Direction::DOWN, 40).ResolveRelativeAddressSelf(0x3, 0x7).GetPtr());
