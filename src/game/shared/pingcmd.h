//=============================================================================//
//
// Purpose: Player ping input commands
//
//=============================================================================//
#ifndef PINGCMD_H
#define PINGCMD_H

#include "mathlib/vector.h"
#include "game/shared/ehandle.h"

enum PingCommandType_e
{
	PING_INVALID = 0,

	PING_QUEUE_TRACE,
	PING_EXECUTE_QUEUED,
	PING_USE_PROMPT,
	PING_ENEMY_SPOTTED,

	// Not a command!!!
	LAST_PING_COMMAND = PING_ENEMY_SPOTTED,
	NUM_PING_COMMANDS = LAST_PING_COMMAND
};

struct PingCommand_s
{
	PingCommand_s()
	{
		Reset();
	}

	void Reset()
	{
		commandType = PING_INVALID;
		pingType = 0;
		entityHandle.Term();
		userTicketId = -1;
		pingOrigin.Init();
	}

	PingCommandType_e commandType;
	int               pingType;
	CBaseHandle       entityHandle;
	int               userTicketId;
	Vector3D          pingOrigin;
};

#endif // PINGCMD_H
