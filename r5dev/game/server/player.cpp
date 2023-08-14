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

	const CClient* client = g_pServer->GetClient(GetEdict() - 1);
	const CNetChan* chan = client->GetNetChan();

	CUserCmd* lastCmd = &m_Commands[MAX_QUEUED_COMMANDS_PROCESS];

	const float maxUnlag = sv_maxunlag->GetFloat();
	const float latencyAmount = Clamp(chan->GetLatency(FLOW_OUTGOING), 0.0f, maxUnlag);
	const float localCurTime = (*g_pGlobals)->m_flCurTime;

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

		// Command issue time from client, note that this value can be altered
		// from the client, and therefore be used to exploit lag compensation.
		const float commandTime = cmd->command_time;
		const float lastCommandTime = m_LastCmd.command_time;

		if (commandTime < lastCommandTime) // Can never be lower than last !!!
		{
			cmd->command_time = localCurTime - latencyAmount;

			if (IsDebug())
			{
				Warning(eDLL_T::SERVER, "%s: cmd->command_time( %f ) < m_LastCmd.command_time( %f ); clamped to %f !!!\n",
					__FUNCTION__, commandTime, lastCommandTime, cmd->command_time);
			}
		}
		else
		{
			const float commandDelta = fabs(commandTime - localCurTime);

			if (commandDelta > maxUnlag) // Too much to unlag, clamp to max !!!
			{
				cmd->command_time = localCurTime - latencyAmount;

				if (IsDebug())
				{
					Warning(eDLL_T::SERVER, "%s: commandDelta( %f ) > maxUnlag( %f ); clamped to %f !!!\n",
						__FUNCTION__, commandDelta, maxUnlag, cmd->command_time);
				}
			}
		}

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