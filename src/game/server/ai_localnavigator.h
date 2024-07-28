//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: AI local navigator
//
// $NoKeywords: $
//=============================================================================//
#ifndef AI_LOCALNAVIGATOR_H
#define AI_LOCALNAVIGATOR_H

#if defined( _WIN32 )
#pragma once
#endif

#include "game/shared/simtimer.h"
#include "ai_component.h"
#include "ai_movetypes.h"
#include "ai_moveprobe.h"

//-----------------------------------------------------------------------------
// CAI_LocalNavigator
//
// Purpose: Handles all the immediate tasks of navigation, independent of
//			path. Implements steering.
//-----------------------------------------------------------------------------
class CAI_LocalNavigator : public CAI_Component,
                           public CAI_ProxyMovementSink
{
	bool m_bLastWasClear;
	char gap_21[7];
	AILocalMoveGoal_t m_LastMoveGoal;
	CSimpleSimTimer m_FullDirectTimer;
	char gap_104[4];
	void* m_pPlaneSolver;
	CAI_MoveProbe* m_pMoveProbe;
};
static_assert(sizeof(CAI_LocalNavigator) == 0x118);

#endif // AI_LOCALNAVIGATOR_H
