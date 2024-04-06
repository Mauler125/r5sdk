//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#ifndef PHYSICS_MAIN_H
#define PHYSICS_MAIN_H

inline void(*v_Physics_RunThinkFunctions)(bool bSimulating);

///////////////////////////////////////////////////////////////////////////////
class VPhysics_Main : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("Physics_RunThinkFunctions", v_Physics_RunThinkFunctions);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("88 4C 24 08 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ?? ?? ?? ??").GetPtr(v_Physics_RunThinkFunctions);
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////

#endif // PHYSICS_MAIN_H
