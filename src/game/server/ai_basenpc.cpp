//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: Base NPC character with AI
//
//=============================================================================//
#include "ai_basenpc.h"
#include "game/shared/util_shared.h"

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAI_Manager* g_AI_Manager = nullptr;

//-------------------------------------

CAI_Manager::CAI_Manager()
{
	m_AIs.EnsureCapacity(MAX_AIS);
}

//-------------------------------------

CAI_BaseNPC** CAI_Manager::AccessAIs()
{
	if (m_AIs.Count())
		return &m_AIs[0];
	return NULL;
}

//-------------------------------------

int CAI_Manager::NumAIs()
{
	return m_AIs.Count();
}

//-------------------------------------

void CAI_Manager::AddAI(CAI_BaseNPC* pAI)
{
	AUTO_LOCK(m_Mutex);
	m_AIs.AddToTail(pAI);
}

//-------------------------------------

void CAI_Manager::RemoveAI(CAI_BaseNPC* pAI)
{
	AUTO_LOCK(m_Mutex);
	const int i = m_AIs.Find(pAI);

	if (i != -1)
		m_AIs.FastRemove(i);
}

//-----------------------------------------------------------------------------

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
