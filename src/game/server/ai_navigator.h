//=============================================================================//
//
// Purpose: AI navigator
//
//=============================================================================//
#ifndef AI_NAVIGATOR_H
#define AI_NAVIGATOR_H

#ifdef _WIN32
#pragma once
#endif

#include "game/shared/predictioncopy.h"
#include "game/shared/simtimer.h"
#include "ai_localnavigator.h"
#include "ai_moveprobe.h"
#include "ai_movetypes.h"
#include "ai_component.h"
#include "ai_network.h"
#include "ai_route.h"

struct ParticleContext // todo: move elsewhere
{
	Vector3D m_curPos;
	Vector3D m_curVel;
	CSimpleSimTimer m_totalSimTime;
	int m_flags;
};


//-----------------------------------------------------------------------------
// CAI_Navigator
//
// Purpose: Implements pathing and path navigation logic
//-----------------------------------------------------------------------------
class CAI_Navigator : public CAI_Component,
                      public CAI_DefMovementSink
{
	typedef CAI_Component BaseClass;
private:
	Navigation_e m_navType;
	bool m_fNavComplete;
	bool m_bLastNavFailed;
	char gap_1e[2];
	CAI_Motor* m_pMotor;
	CAI_MoveProbe* m_pMoveProbe;
	CAI_LocalNavigator* m_pLocalNavigator;
	CAI_Network* m_pAINetwork;
	CAI_Path* m_pPath;
	char m_utilityPath[32];
	CAI_WaypointList* m_pClippedWaypoints;
	float m_flTimeClipped;
	int m_PreviousMoveActivity;
	int m_PreviousArrivalActivity;
	int m_moveTransitionAnim;
	ParticleContext m_particleContext;
	Vector3D m_wallRunSurfacePos;
	Vector3D m_wallRunMoveDir;
	unsigned short m_wallRunHintIdx; // The wallrun hint index of the navmesh.
	unsigned short m_wallRunTriangle;
	bool m_bValidateActivitySpeed;
	bool m_bCalledStartMove;
	bool m_bAdjustMoveSpeedToSquad;
	bool m_bForcedSimplify;
	float m_flNextSimplifyTime;
	float m_flLastSuccessfulSimplifyTime;
	float m_flTimeLastAvoidanceTriangulate;
	float m_timePathRebuildMax;
	float m_timePathRebuildDelay;
	float m_timePathRebuildFail;
	float m_timePathRebuildNext;
	int m_prevPathCorner;
	int m_nextPathCorner;
	bool m_savePathCornerOnScheduleChange;
	bool m_pathCornerDirectionForward;
	bool m_fRememberStaleNodes;
	bool m_bNoPathcornerPathfinds;
	float m_checkClusterDangerAtTime;
	bool m_bHadPath;
	bool m_bAnimWasMoving;
	bool m_setConditionPathInvolvesDangerousCluster;
	bool m_bProbeHitPhysicsObject;
	bool m_bPathBlockedByPhysicsObject;
	bool m_bPathBlockedByNPC;
	bool m_bProbeHitNPC;
	bool m_fPeerMoveWait;
	EHANDLE m_hPeerWaitingOn;
	CSimTimer m_PeerWaitMoveTimer;
	CSimTimer m_PeerWaitClearTimer;
	CSimTimer m_NextSidestepTimer;
	EHANDLE m_hBigStepGroundEnt;
	EHANDLE m_hLastBlockingEnt;
	EHANDLE m_hAvoidEnt;
	float m_avoidDistSqr;
	Vector3D m_vPosBeginFailedSteer;
	float m_timeBeginFailedSteer;
	float m_traverseRefYaw;
	Vector3D m_traverseRefPos;
	Vector3D m_traversePlantPos;
	int m_nNavFailCounter;
	float m_flLastNavFailTime;
	float m_flLastPathFindTime;
	int m_moveFlags;
};

static_assert(sizeof(CAI_Navigator) == 0x160);

#endif // AI_NAVIGATOR_H