//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: AI Waypoint
//
// $NoKeywords: $
//=============================================================================//
#ifndef AI_WAYPOINT_H
#define AI_WAYPOINT_H

#if defined( _WIN32 )
#pragma once
#endif

#include "game/shared/predictioncopy.h"

struct AI_Waypoint_t
{
	//---------------------------------
	//
	// Basic info
	//
	Vector3D vecLocation;
	float flYaw;
	int iNodeID;
	int polyRef;
	unsigned char jumpType;
	char gap_19[3];

	//---------------------------------
	//
	// If following a designer laid path, the path-corner entity (if any)
	//
	EHANDLE hPathCorner;

private:
	int waypointFlags; // See WaypointFlags_t
	Navigation_e navType;
	AI_Waypoint_t* pNext;
	AI_Waypoint_t* pPrev;
};
static_assert(sizeof(AI_Waypoint_t) == 0x38);

// todo: implement the class
class CAI_WaypointList
{
	AI_Waypoint_t* m_pFirstWaypoint;
};
static_assert(sizeof(CAI_WaypointList) == 8);

#endif // AI_WAYPOINT_H
