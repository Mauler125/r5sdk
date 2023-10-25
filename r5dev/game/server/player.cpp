//======== Copyright (c) Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//
#include "core/stdafx.h"
#include "common/protocol.h"
#include "game/shared/shareddefs.h"
#include "game/shared/usercmd.h"
#include "game/server/movehelper_server.h"
#include "gameinterface.h"
#include "player.h"

#include "engine/server/server.h"

//------------------------------------------------------------------------------
// Purpose: executes a null command for this player
//------------------------------------------------------------------------------
void CPlayer::RunNullCommand(void)
{
	CUserCmd cmd;

	float flOldFrameTime = (*g_pGlobals)->m_flFrameTime;
	float flOldCurTime = (*g_pGlobals)->m_flCurTime;

	cmd.frametime = flOldFrameTime;
	cmd.command_time = flOldCurTime;

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
	float flTime = float(TIME_TO_TICKS(flTimeBase));

	if (flTime < 0.0f)
		flTime = 0.0f;

	SetLastUCmdSimulationRemainderTime(flTime);

	float flSimulationTime = flTimeBase - m_lastUCmdSimulationRemainderTime * (*g_pGlobals)->m_flTickInterval;
	if (flSimulationTime >= 0.0f)
	{
		flTime = flSimulationTime;
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
// Purpose: clamps the unlag amount to sv_unlag + clockdrift
// Input  : *cmd - 
//------------------------------------------------------------------------------
void CPlayer::ClampUnlag(CUserCmd* cmd)
{
	const CClient* client = g_pServer->GetClient(GetEdict() - 1);
	const CNetChan* chan = client->GetNetChan();

	const float clockDriftMsecs = sv_clockcorrection_msecs->GetFloat() / 1000.0f;
	const float maxUnlag = sv_maxunlag->GetFloat();
	const float latencyAmount = Clamp(chan->GetLatency(FLOW_OUTGOING), 0.0f, maxUnlag);
	const float serverTime = (*g_pGlobals)->m_flCurTime;

	// Command issue time from client, note that this value can be altered
	// from the client, and therefore be used to exploit lag compensation.
	const float commandTime = cmd->command_time;
	const float lastCommandTime = m_LastCmd.command_time;
	const float commandDelta = fabs(commandTime - serverTime);

	bool recomputeUnlag = false;

	// Check delta first, otherwise player could set commandTime to a fixed
	// time and circumvent the system, as commandTime < lastCommandTime or
	// commandTime > localCurTime will always fail.
	if (commandDelta > maxUnlag)
	{
		// Too much to unlag, clamp to max !!!
		recomputeUnlag = true;
		DevWarning(eDLL_T::SERVER, "%s: commandDelta( %f ) > maxUnlag( %f ) !!!\n",
			__FUNCTION__, commandDelta, maxUnlag);
	}
	else if (commandTime < (lastCommandTime - clockDriftMsecs))
	{
		// Can never be lower than last !!!
		recomputeUnlag = true;
		DevWarning(eDLL_T::SERVER, "%s: cmd->command_time( %f ) < (m_LastCmd.command_time( %f ) - clockDriftMsecs( %f )) !!!\n",
			__FUNCTION__, commandTime, lastCommandTime, clockDriftMsecs);
	}
	else if (commandTime > (serverTime + clockDriftMsecs))
	{
		// Too far in the future, clamp to max !!!
		recomputeUnlag = true;
		DevWarning(eDLL_T::SERVER, "%s: cmd->command_time( %f ) > (g_pGlobals->m_flCurTime( %f ) + clockDriftMsecs( %f )) !!!\n",
			__FUNCTION__, commandTime, serverTime, clockDriftMsecs);
	}

	if (recomputeUnlag)
	{
		// Clamp it to server time minus latency. Note that it could still
		// be lower than previous, hence the clamp on the recomputation.
		float newCommandTime = Clamp(serverTime - latencyAmount, lastCommandTime, serverTime);
		cmd->command_time = newCommandTime;

		DevWarning(eDLL_T::SERVER, "%s: Clamped cmd->command_time( %f ) to %f !!!\n",
			__FUNCTION__, commandTime, newCommandTime);
	}
}

//------------------------------------------------------------------------------
// Purpose: processes user cmd's for this player
// Input  : *cmds - 
//			numCmds - 
//			totalCmds - 
//			droppedPackets - 
//			paused - 
//------------------------------------------------------------------------------
void CPlayer::ProcessUserCmds(CUserCmd* cmds, int numCmds, int totalCmds,
	int droppedPackets, bool paused)
{
	if (totalCmds <= 0)
		return;

	CUserCmd* lastCmd = &m_Commands[MAX_QUEUED_COMMANDS_PROCESS];

	for (int i = totalCmds - 1; i >= 0; i--)
	{
		CUserCmd* cmd = &cmds[i];
		const int commandNumber = cmd->command_number;

		if (commandNumber <= m_latestCommandQueued)
			continue;

		m_latestCommandQueued = commandNumber;
		const int lastCommandNumber = lastCmd->command_number;

		if (lastCommandNumber == MAX_QUEUED_COMMANDS_PROCESS)
			return;

		if (sv_unlag_clamp->GetBool())
			ClampUnlag(cmd);

		CUserCmd* queuedCmd = &m_Commands[lastCommandNumber];
		queuedCmd->Copy(cmd);

		if (++lastCmd->command_number > player_userCmdsQueueWarning->GetInt())
		{
			const float curTime = float(Plat_FloatTime());

			if ((curTime - m_lastCommandCountWarnTime) > 0.5f)
				m_lastCommandCountWarnTime = curTime;
		}
	}

	lastCmd->tick_count += droppedPackets;
	m_bGamePaused = paused;
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