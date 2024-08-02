//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: AI locomotion
//
// $NoKeywords: $
//=============================================================================//
#ifndef AI_MOTOR_H
#define AI_MOTOR_H

#ifdef _WIN32
#pragma once
#endif

#include "ai_component.h"
#include "ai_movetypes.h"
#include "ai_moveprobe.h"
#include "ai_interest.h"

#include "recast/Detour/Include/DetourNavMeshQuery.h"

//-----------------------------------------------------------------------------
// CAI_Motor
//
// Purpose: Implements the primitive locomotion of AIs. 
//-----------------------------------------------------------------------------

class CAI_Motor : public CAI_Component,
                  public CAI_ProxyMovementSink
{
    Vector3D m_steerDirection;
    dtStraightPathResult m_straightPathCached;
    Vector3D m_pushedVel;
    float m_flMoveInterval;
    float m_IdealYaw;
    float m_fMoveYaw;
    Vector3D m_vecVelocity;
    Vector3D m_vecAngularVelocity;
    bool m_bMoving;
    char gap_bd[3];
    float m_moveSpeedScale;
    float m_moveSpeedScaleScript;
    int m_nDismountSequence;
    Vector3D m_vecDismount;
    CAI_InterestTarget m_facingQueue;
    CAI_MoveProbe* m_pMoveProbe;
};
static_assert(sizeof(CAI_Motor) == 0x100);

//=============================================================================//

#endif // AI_MOTOR_H