//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: AI move types
//
// $NoKeywords: $
//=============================================================================//
#ifndef AI_MOVETYPES_H
#define AI_MOVETYPES_H

#if defined( _WIN32 )
#pragma once
#endif

#include "ai_navtypes.h"

class CAI_Path;

//-------------------------------------

struct MovementTurn
{
	sharedactivity_e turnAct;
	int              sequence;
	float            forwardDistance;
	float            perpendicularDistance;
	float            forwardMoveAdjust;
};
static_assert(sizeof(MovementTurn) == 0x14);

//-------------------------------------

enum AIMoveResult_t
{
	AIMR_BLOCKED_ENTITY         = -1,           // Move was blocked by an entity
	AIMR_BLOCKED_WORLD          = -2,           // Move was blocked by the world
	AIMR_BLOCKED_NPC            = -3,           // Move was blocked by an NPC
	AIMR_ILLEGAL                = -4,           // Move is illegal for some reason

	AIMR_OK                     = 0,
	
	AIMR_CHANGE_TYPE,                           // Locomotion method has changed
};

//-------------------------------------

struct AIMoveTrace_t
{
	AIMoveResult_t fStatus;            // See AIMoveResult_t
	Vector3D       vEndPosition;       // The last point that could be moved to
	Vector3D       vHitNormal;         // The normal of a hit, if any. vec3_origin if none. Can be none even if "hit"
	CBaseEntity*   pObstruction;       // The obstruction I bumped into (if any)
	float          flTotalDist;
	float          flDistObstructed;
	Vector3D       vJumpVelocity;
	float          flStepUpDistance;
};
static_assert(sizeof(AIMoveTrace_t) == 0x40);

//-------------------------------------

struct AILocalMoveGoal_t
{
	// Object of the goal
	Vector3D target;

	// The actual move. Note these need not always agree with "target"
	Vector3D dir;
	Vector3D facing;
	float speed;

	// The distance maximum distance intended to travel in path length
	float maxDist;

	// The distance expected to move this think
	float curExpectedDist;

	Navigation_e navType;
	CBaseEntity* pMoveTarget;

	unsigned int flags;

	// The path from which this goal was derived
	CAI_Path* pPath;

	// The result if a forward probing trace has been done
	bool bHasTraced;
	AIMoveTrace_t directTrace;
	AIMoveTrace_t thinkTrace;
};
static_assert(sizeof(AILocalMoveGoal_t) == 0xD8);

//-----------------------------------------------------------------------------
// Purpose: The set of callbacks used by lower-level movement classes to
//			notify and receive guidance from higher level-classes
//-----------------------------------------------------------------------------

abstract_class IAI_MovementSink
{
public: // TODO: reverse engineer interface.
	//---------------------------------
	//
	// Queries
	//
	virtual float CalcYawSpeed( void ) = 0;
};

class CAI_DefMovementSink : public IAI_MovementSink
{
	//---------------------------------
	//
	// Queries
	//
	virtual float CalcYawSpeed(void) { return -1.0; } // might be incorrect!
};

class CAI_ProxyMovementSink : public CAI_DefMovementSink
{
public:
	CAI_ProxyMovementSink()
		: m_pProxied(NULL)
	{
	}

	//---------------------------------

	void Init(IAI_MovementSink* pMovementServices) { m_pProxied = pMovementServices; }

private:
	IAI_MovementSink* m_pProxied;
};

//=============================================================================

#endif // AI_MOVETYPES_H
