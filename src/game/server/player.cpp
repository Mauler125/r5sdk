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

// NOTE[ AMOS ]: default tick interval (0.05) * default cvar value (10) = total time buffer of 0.5, which is the default of cvar 'sv_maxunlag'.
static ConVar sv_maxUserCmdProcessTicks("sv_maxUserCmdProcessTicks", "10", FCVAR_NONE, "Maximum number of client-issued UserCmd ticks that can be replayed in packet loss conditions, 0 to allow no restrictions.");

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
	return CPlayer__EyeAngles(this, pAngles);
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

	float flSimulationTime = flTimeBase - m_lastUCmdSimulationRemainderTime * TICK_INTERVAL;
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
// TODO: this code is experimental and has reported problems from players with
// high latency, needs to be debugged or a different approach needs to be taken!
// Defaulted to OFF for now
static ConVar sv_unlag_clamp("sv_unlag_clamp", "0", FCVAR_RELEASE, "Clamp the difference between the current time and received command time to sv_maxunlag + sv_clockcorrection_msecs.");

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

		if (sv_unlag_clamp.GetBool())
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
	CPlayer__PlayerRunCommand(this, pUserCmd, pMover);
}

//------------------------------------------------------------------------------
// Purpose: stores off a user command
// Input  : *pUserCmd - 
//------------------------------------------------------------------------------
void CPlayer::SetLastUserCommand(CUserCmd* pUserCmd)
{
	m_LastCmd.Copy(pUserCmd);
}

//------------------------------------------------------------------------------
// Purpose: run physics simulation for player
// Input  : *player (this) - 
//			numPerIteration - 
//			adjustTimeBase - 
//------------------------------------------------------------------------------
bool Player_PhysicsSimulate(CPlayer* player, int numPerIteration, bool adjustTimeBase)
{
	CClientExtended* const cle = g_pServer->GetClientExtended(player->GetEdict() - 1);
	const int numUserCmdProcessTicksMax = sv_maxUserCmdProcessTicks.GetInt();

	if (numUserCmdProcessTicksMax && (*g_pGlobals)->m_nGameMode != GameMode_t::SP_MODE) // don't apply this filter in SP games
		cle->InitializeMovementTimeForUserCmdProcessing(numUserCmdProcessTicksMax, TICK_INTERVAL);
	else // Otherwise we don't care to track time
		cle->SetRemainingMovementTimeForUserCmdProcessing(FLT_MAX);

	return CPlayer__PhysicsSimulate(player, numPerIteration, adjustTimeBase);
}

/*
=====================
CC_CreateFakePlayer_f

  Creates a fake player
  on the server
=====================
*/
static void CC_CreateFakePlayer_f(const CCommand& args)
{
	if (!g_pServer->IsActive())
		return;

	if (args.ArgC() < 3)
	{
		Msg(eDLL_T::SERVER, "usage 'sv_addbot': name(string) teamid(int)\n");
		return;
	}

	const int numPlayers = g_pServer->GetNumClients();

	// Already at max, don't create.
	if (numPlayers >= g_ServerGlobalVariables->m_nMaxClients)
		return;

	const char* playerName = args.Arg(1);

	int teamNum = atoi(args.Arg(2));
	const int maxTeams = int(g_pServer->GetMaxTeams()) + 1;

	// Clamp team count, going above the limit will
	// cause a crash. Going below 0 means that the
	// engine will assign the bot to the last team.
	if (teamNum > maxTeams)
		teamNum = maxTeams;

	g_pEngineServer->LockNetworkStringTables(true);

	const edict_t nHandle = g_pEngineServer->CreateFakeClient(playerName, teamNum);
	g_pServerGameClients->ClientFullyConnect(nHandle, false);

	g_pEngineServer->LockNetworkStringTables(false);
}

static ConCommand sv_addbot("sv_addbot", CC_CreateFakePlayer_f, "Creates a bot on the server", FCVAR_RELEASE);

void VPlayer::Detour(const bool bAttach) const
{
	DetourSetup(&CPlayer__PhysicsSimulate, &Player_PhysicsSimulate, bAttach);
}
