//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#ifndef PHYSICS_MAIN_H
#define PHYSICS_MAIN_H

inline CMemory p_Physics_RunThinkFunctions;
inline auto v_Physics_RunThinkFunctions = p_Physics_RunThinkFunctions.RCast<void (*)(bool bSimulating)>();

void Physics_Main_Attach();
void Physics_Main_Detach();
///////////////////////////////////////////////////////////////////////////////
class VPhysics_Main : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: Physics_RunThinkFunctions            : {:#18x} |\n", p_Physics_RunThinkFunctions.GetPtr());
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		p_Physics_RunThinkFunctions = g_GameDll.FindPatternSIMD("88 4C 24 08 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ?? ?? ?? ??");
		v_Physics_RunThinkFunctions = p_Physics_RunThinkFunctions.RCast<void (*)(bool)>();
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VPhysics_Main);
#endif // PHYSICS_MAIN_H
