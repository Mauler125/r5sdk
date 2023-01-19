//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: Physics simulation for non-havok/ipion objects
//
// $NoKeywords: $
//=============================================================================//
#include "core/stdafx.h"
#include "player.h"
#include "physics_main.h"

//-----------------------------------------------------------------------------
// Purpose: Runs the main physics simulation loop against all entities ( except players )
//-----------------------------------------------------------------------------
void Physics_RunThinkFunctions(bool bSimulating)
{
	v_Physics_RunThinkFunctions(bSimulating);
}

///////////////////////////////////////////////////////////////////////////////
void Physics_Main_Attach()
{
	DetourAttach(&v_Physics_RunThinkFunctions, &Physics_RunThinkFunctions);
}

void Physics_Main_Detach()
{
	DetourDetach(&v_Physics_RunThinkFunctions, &Physics_RunThinkFunctions);
}