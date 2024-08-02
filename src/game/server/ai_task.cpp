//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: Sets up the tasks for default AI.
//
// $NoKeywords: $
//=============================================================================//
//#include "ai_basenpc.h"
#include "ai_task.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static const char* const s_ppszTaskFailureText[NUM_FAIL_CODES] =
{
	"No failure",                                    // NO_TASK_FAILURE
	"No target",                                     // FAIL_NO_TARGET
	"Weapon owned by someone else",                  // FAIL_WEAPON_OWNED
	"Item doesn't exist",                            // FAIL_ITEM_NO_FIND
	"No hint node",                                  // FAIL_NO_HINT_NODE
	"Schedule not found",                            // FAIL_SCHEDULE_NOT_FOUND
	"Don't have an enemy",                           // FAIL_NO_ENEMY
	"Found no near node",                            // FAIL_NO_NEAR_NODE
	"Found no back away node",                       // FAIL_NO_BACKAWAY_NODE
	"Couldn't find cover",                           // FAIL_NO_COVER
	"Couldn't find flank",                           // FAIL_NO_FLANK
	"Couldn't find shoot position",                  // FAIL_NO_SHOOT
	"Don't have a route",                            // FAIL_NO_ROUTE
	"Don't have a route: no goal",                   // FAIL_NO_ROUTE_GOAL
	"Don't have a route: blocked",                   // FAIL_NO_ROUTE_BLOCKED
	"Don't have a route: illegal move",              // FAIL_NO_ROUTE_ILLEGAL
	"Couldn't walk to target",                       // FAIL_NO_WALK
	"Don't have LOS",                                // FAIL_NO_LOS
	"Node already locked",                           // FAIL_ALREADY_LOCKED
	"No sound present",                              // FAIL_NO_SOUND
	"Bad activity",                                  // FAIL_BAD_ACTIVITY
	"No goal entity",                                // FAIL_NO_GOAL
	"No player",                                     // FAIL_NO_PLAYER
	"Can't reach any nodes",                         // FAIL_NO_REACHABLE_NODE
	"No AI Network to use",                          // FAIL_NO_AI_NETWORK
	"No start position",                             // FAIL_NO_START_POSITION
	"Bad position to target",                        // FAIL_BAD_POSITION
	"Route destination no longer valid",             // FAIL_BAD_PATH_GOAL
	"Stuck on top of something",                     // FAIL_STUCK_ONTOP
	"Item has been taken",                           // FAIL_ITEM_TAKEN
	"Too frozen",                                    // FAIL_FROZEN
	"Animation blocked",                             // FAIL_ANIMATION_BLOCKED
	"Timeout",                                       // FAIL_TIMEOUT
	"Detour specific",                               // FAIL_DETOUR_SPECIFIC
};

const char* TaskFailureToString(const AI_TaskFailureCode_t code)
{
	const char* pszResult;
	if (code < 0 || code >= NUM_FAIL_CODES)
		pszResult = (const char*)code;
	else
		pszResult = s_ppszTaskFailureText[code];
	return pszResult;
}
