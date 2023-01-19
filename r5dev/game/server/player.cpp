//======== Copyright (c) Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//
#include "core/stdafx.h"
#include "player.h"
#include "gameinterface.h"
#include "game/shared/shareddefs.h"

//------------------------------------------------------------------------------
// Purpose: executes a null command for this player
//------------------------------------------------------------------------------
void CPlayer::RunNullCommand(void)
{
	float flOldFrameTime = g_pGlobals->m_fFrameTime;
	float flOldCurTime = g_pGlobals->m_fCurTime;

	pl.fixangle = FIXANGLE_NONE;

	SetTimeBase(g_pGlobals->m_fCurTime);


	// !TODO: Run command..

	g_pGlobals->m_fFrameTime = flOldFrameTime;
	g_pGlobals->m_fCurTime = flOldCurTime;
}

//------------------------------------------------------------------------------
// Purpose: gets the eye angles of this player
// Input  : &angles - 
// Output : QAngle*
//------------------------------------------------------------------------------
QAngle* CPlayer::EyeAngles(QAngle& angles)
{
	return v_CPlayer__EyeAngles(this, &angles);
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

	float flSomeTime = flTimeBase - m_lastUCmdSimulationRemainderTime * g_pGlobals->m_nTickInterval;
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
			_InterlockedOr16(g_pGlobals->m_pUnk0 + nEdict + 32, 0x200u);
		}

		m_totalExtraClientCmdTimeAttempted = flRemainderTime;
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
			_InterlockedOr16(g_pGlobals->m_pUnk0 + nEdict + 32, 0x200u);
		}

		m_totalExtraClientCmdTimeAttempted = flAttemptedTime;
	}
}
