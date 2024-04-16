//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "engine/server/server.h"
#include "engine/client/client.h"

#include "player_command.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPlayerMove::CPlayerMove(void)
{
}

//-----------------------------------------------------------------------------
// Purpose: Runs movement commands for the player
// Input  : *player - 
//			*ucmd - 
//			*moveHelper - 
//-----------------------------------------------------------------------------
void CPlayerMove::StaticRunCommand(CPlayerMove* thisp, CPlayer* player, CUserCmd* ucmd, IMoveHelper* moveHelper)
{
	CClientExtended* const cle = g_pServer->GetClientExtended(player->GetEdict() - 1);
	float playerFrameTime;
	
	// Always default to clamped UserCmd frame time if this cvar is set
	if (player_disallow_negative_frametime->GetBool())
		playerFrameTime = fmaxf(ucmd->frametime, 0.0f);
	else
	{
		if (player->m_bGamePaused)
			playerFrameTime = 0.0f;
		else
			playerFrameTime = TICK_INTERVAL;

		if (ucmd->frametime)
			playerFrameTime = ucmd->frametime;
	}

	if (sv_clampPlayerFrameTime->GetBool() && player->m_joinFrameTime > ((*g_pflServerFrameTimeBase) + playerframetimekick_margin->GetFloat()))
		playerFrameTime = 0.0f;

	const float timeAllowedForProcessing = cle->ConsumeMovementTimeForUserCmdProcessing(playerFrameTime);

	if (!player->IsBot() && (timeAllowedForProcessing < playerFrameTime))
		return; // Don't process this command

	CPlayerMove__RunCommand(thisp, player, ucmd, moveHelper);
}

void VPlayerMove::Detour(const bool bAttach) const
{
	DetourSetup(&CPlayerMove__RunCommand, &CPlayerMove::StaticRunCommand, bAttach);
}
