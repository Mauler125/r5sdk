//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: AI Tactical Services
//
//=============================================================================//
#ifndef AI_TACTICALSERVICES_H
#define AI_TACTICALSERVICES_H

#if defined( _WIN32 )
#pragma once
#endif

#include "ai_component.h"

class CAI_TacticalServices : public CAI_Component
{
private:
	int             m_LOSSearchDataIndex;
	int             m_strafeActivity;
	bool            m_firstIterationOfFindLos;
	char gap_19[7];
	CAI_Network*    m_pNetwork;
	CAI_Pathfinder* m_pPathfinder;
	bool            m_prevLOSCheckSuccess;
	char gap_31[3];
	float           m_prevLOSCheckTime;
	Vector3D        m_prevThreatPos;
	Vector3D        m_prevSearchPos;
};
static_assert(sizeof(CAI_TacticalServices) == 0x50);

#endif // AI_TACTICALSERVICES_H
