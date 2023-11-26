//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#ifndef PHYSICS_MAIN_H
#define PHYSICS_MAIN_H

inline CMemory p_Physics_RunThinkFunctions;
inline void(*v_Physics_RunThinkFunctions)(bool bSimulating);

///////////////////////////////////////////////////////////////////////////////
class VPhysics_Main : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("Physics_RunThinkFunctions", p_Physics_RunThinkFunctions.GetPtr());
	}
	virtual void GetFun(void) const
	{
		p_Physics_RunThinkFunctions = g_GameDll.FindPatternSIMD("88 4C 24 08 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ?? ?? ?? ??");
		v_Physics_RunThinkFunctions = p_Physics_RunThinkFunctions.RCast<void (*)(bool)>();
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////

#endif // PHYSICS_MAIN_H
