//======== Copyright (c) Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//
#include "core/stdafx.h"
#include "player.h"
#include "gameinterface.h"
#include "game/shared/shareddefs.h"
#include "game/shared/usercmd.h"
#include "game/server/movehelper_server.h"

//------------------------------------------------------------------------------
// Purpose: executes a null command for this player
//------------------------------------------------------------------------------
void CPlayer::RunNullCommand(void)
{
	CUserCmd cmd;

	float flOldFrameTime = (*g_pGlobals)->m_flFrameTime;
	float flOldCurTime = (*g_pGlobals)->m_flCurTime;

	pl.fixangle = FIXANGLE_NONE;
	EyeAngles(&cmd.viewangles);

	SetTimeBase((*g_pGlobals)->m_flCurTime);
	MoveHelperServer()->SetHost(this);

	PlayerRunCommand(&cmd, MoveHelperServer());
	SetLastUserCommand(&cmd);

	(*g_pGlobals)->m_flFrameTime = flOldFrameTime;
	(*g_pGlobals)->m_flCurTime = flOldCurTime;

	MoveHelperServer()->SetHost(NULL);
}

//------------------------------------------------------------------------------
// Purpose: gets the eye angles of this player
// Input  : *pAngles - 
// Output : QAngle*
//------------------------------------------------------------------------------
QAngle* CPlayer::EyeAngles(QAngle* pAngles)
{
	return v_CPlayer__EyeAngles(this, pAngles);
}

//------------------------------------------------------------------------------
// Purpose: sets the time base for this player
// Input  : flTimeBase - 
//------------------------------------------------------------------------------
inline void CPlayer::SetTimeBase(float flTimeBase)
{
	float flTime = TIME_TO_TICKS(flTimeBase);

	if (flTime < 0.0f)
		flTime = 0.0f;

	SetLastUCmdSimulationRemainderTime(flTime);

	float flSomeTime = flTimeBase - m_lastUCmdSimulationRemainderTime * (*g_pGlobals)->m_flTickInterval;
	if (flSomeTime >= 0.0)
	{
		flTime = flSomeTime;
	}

	SetTotalExtraClientCmdTimeAttempted(flTime);
}

//------------------------------------------------------------------------------
// Purpose: sets the last user cmd simulation remainder time
// Input  : flRemainderTime - 
//------------------------------------------------------------------------------
void CPlayer::SetLastUCmdSimulationRemainderTime(float flRemainderTime)
{
	if (m_lastUCmdSimulationRemainderTime != flRemainderTime)
	{
		edict_t nEdict = NetworkProp()->GetEdict();
		if (nEdict != FL_EDICT_INVALID)
		{
			_InterlockedOr16((SHORT*)(*g_pGlobals)->m_pEdicts + nEdict + 32, 0x200u);
		}

		m_lastUCmdSimulationRemainderTime = flRemainderTime;
	}
}

//------------------------------------------------------------------------------
// Purpose: sets the total extra client cmd time attempted
// Input  : flAttemptedTime - 
//------------------------------------------------------------------------------
void CPlayer::SetTotalExtraClientCmdTimeAttempted(float flAttemptedTime)
{
	if (m_totalExtraClientCmdTimeAttempted != flAttemptedTime)
	{
		edict_t nEdict = NetworkProp()->GetEdict();
		if (nEdict != FL_EDICT_INVALID)
		{
			_InterlockedOr16((SHORT*)(*g_pGlobals)->m_pEdicts + nEdict + 32, 0x200u);
		}

		m_totalExtraClientCmdTimeAttempted = flAttemptedTime;
	}
}

//------------------------------------------------------------------------------
// Purpose: runs user command for this player
// Input  : *pUserCmd - 
//			*pMover - 
//------------------------------------------------------------------------------
void CPlayer::PlayerRunCommand(CUserCmd* pUserCmd, IMoveHelper* pMover)
{
	v_CPlayer__PlayerRunCommand(this, pUserCmd, pMover);
}

//------------------------------------------------------------------------------
// Purpose: stores off a user command
// Input  : *pUserCmd - 
//------------------------------------------------------------------------------
void CPlayer::SetLastUserCommand(CUserCmd* pUserCmd)
{
	m_LastCmd.Copy(pUserCmd);
}