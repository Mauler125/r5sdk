//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: Base NPC character with AI
//
//=============================================================================//
#include "ai_basenpc.h"
#include "game/shared/util_shared.h"


static ConVar ai_debug_tasks("ai_debug_tasks", "0", FCVAR_DEVELOPMENTONLY, "Debug the attempted tasks");

void CAI_BaseNPC::_TaskFail(CAI_BaseNPC* thisptr, const AI_TaskFailureCode_t code)
{
	if (ai_debug_tasks.GetBool())
	{
		thisptr->m_failText = TaskFailureToString(code);
		thisptr->m_failedSchedule = thisptr->GetCurSchedule();

		Msg(eDLL_T::SERVER, "TaskFail -> %s (%s)\n", thisptr->m_failText, UTIL_GetEntityScriptInfo(thisptr));
	}

	CAI_BaseNPC__TaskFail(thisptr, code);
}

void VAI_BaseNPC::Detour(const bool bAttach) const
{
	DetourSetup(&CAI_BaseNPC__TaskFail, &CAI_BaseNPC::_TaskFail, bAttach);
}
