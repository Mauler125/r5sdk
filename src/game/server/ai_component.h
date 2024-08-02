//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: Base AI component
//
//=============================================================================//

#ifndef AI_COMPONENT_H
#define AI_COMPONENT_H

#if defined( _WIN32 )
#pragma once
#endif

class CAI_BaseNPC;

//-----------------------------------------------------------------------------
// CAI_Component
//
// Purpose: Shared functionality of all classes that assume some of the 
//			responsibilities of an owner AI. 
//-----------------------------------------------------------------------------

class CAI_Component
{
	virtual void SetOuter(CAI_BaseNPC* pOuter) { m_pOuter = pOuter; }
	// todo: implement the rest
private:
	CAI_BaseNPC* m_pOuter;
};
static_assert(sizeof(CAI_Component) == 16);

#endif // AI_COMPONENT_H
