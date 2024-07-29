//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: AI route
//
// $NoKeywords: $
//=============================================================================//
#ifndef AI_ROUTE_H
#define AI_ROUTE_H

#ifdef _WIN32
#pragma once
#endif

#include "ai_component.h"
#include "ai_movetypes.h"
#include "ai_waypoint.h"

class CAI_Path : public CAI_Component
{
public:
	CAI_WaypointList m_Waypoints;
	char m_clusterPath[32];
	char m_clusterPathNoExclusions[32];
	float m_goalTolerance;
	sharedactivity_e m_activity;
	int m_sequence;
	int m_scriptMoveSequence;
	int m_target;
	float m_waypointTolerance;
	bool m_movementTurnsAreValid;
	char gap_71[3];
	MovementTurn m_leftTurn;
	MovementTurn m_rightTurn;
	sharedactivity_e m_arrivalActivity;
	int m_arrivalSequence;
	sharedactivity_e m_animArrivalAct;
	int m_animArrivalSequence;
	float m_animArrivalDist;
	float m_animArrivalYaw;
	float m_animArrivalYawOffset;
	Vector3D m_animArrivalIdealStartPosition;
	int m_animArrivalFlags;
	bool m_animArrivalFail;
	char gap_c9[3];
	int m_iLastNodeReached;
	int m_goalNode;
	int m_startGroundEnt;
	int m_goalGroundEnt;
	bool m_bUsedSquadCachedPath;
	char gap_dd[3];
	Vector3D m_goalPos;
	Vector3D m_goalPos_worldSpaceCached;
	int m_goalType;
	int m_goalFlags;
	float m_routeStartTime;
	float m_goalStoppingDistance;
};
static_assert(sizeof(CAI_Path) == 0x108);

#endif // AI_ROUTE_H