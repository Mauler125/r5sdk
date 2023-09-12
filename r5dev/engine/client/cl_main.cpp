//=============================================================================//
//
// Purpose: 
//
//=============================================================================//

#include "engine/host.h"
#include "clientstate.h"
#include "cl_splitscreen.h"
#include "cl_main.h"
#include "engine/net.h"
#include "cdll_engine_int.h"
#include "windows/id3dx.h"
#include "geforce/reflex.h"

static float s_lastMovementCall = 0.0;
static float s_LastFrameTime = 0.0;

// The game supports sending multiple movement frames per simulation tick,
// therefore we need to track when the last call was, and make sure we only
// call these latency markers once per engine frame.
static int s_LastMovementReflexFrame = -1;

//-----------------------------------------------------------------------------
// Purpose: run client's movement frame
//-----------------------------------------------------------------------------
void CL_MoveEx()
{
	CClientState* cl = GetBaseLocalClient();

	if (!cl->IsConnected())
		return;

	if (!v_Host_ShouldRun())
		return;

	int commandTick = -1;

	if (cl->m_CurrFrameSnapshot)
		commandTick = cl->m_CurrFrameSnapshot->m_TickUpdate.m_nCommandTick;

	bool sendPacket = true;
	CNetChan* chan = cl->m_NetChannel;

	// Only perform clamping and packeting if the timescale value is default,
	// else the timescale change won't be handled in the player's movement.
	const float hostTimeScale = host_timescale->GetFloat();
	const bool isTimeScaleDefault = hostTimeScale == 1.0;

	const float minFrameTime = usercmd_frametime_min->GetFloat();
	const float maxFrameTime = usercmd_frametime_max->GetFloat();

	const float netTime = float(*g_pNetTime);

	if (cl->m_flNextCmdTime <= (maxFrameTime * 0.5f) + netTime)
		sendPacket = chan->CanPacket();

	else if (cl->m_nOutgoingCommandNr - (commandTick+1) < MAX_BACKUP_COMMANDS || isTimeScaleDefault)
		sendPacket = false;

	const bool isActive = cl->IsActive();

	if (isActive)
	{
		const float movementCallTime = float(Plat_FloatTime());
		const int outCommandNr = cl->m_nOutgoingCommandNr;

		const bool isPaused = cl->IsPaused();
		const int nextCommandNr = isPaused ? outCommandNr : outCommandNr+1;

		FOR_EACH_SPLITSCREEN_PLAYER(i)
		{
			if (g_pSplitScreenMgr->IsDisconnecting(i))
				continue;

			float frameTime = 0.0f;

			if (cl_move_use_dt->GetBool())
			{
				float timeScale;
				float deltaTime;

				if (isPaused)
				{
					timeScale = 1.0f;
					frameTime = movementCallTime - s_lastMovementCall;
					deltaTime = frameTime;
				}
				else
				{
					timeScale = hostTimeScale;
					frameTime = cl->m_flFrameTime + s_LastFrameTime;
					deltaTime = frameTime / timeScale;
				}

				// Clamp the frame time to the maximum.
				if (deltaTime > maxFrameTime)
					frameTime = timeScale * maxFrameTime;

				// Drop this frame if delta time is below the minimum.
				const bool dropFrame = (isTimeScaleDefault && deltaTime < minFrameTime);

				// This check originally was 'time < 0.0049999999', but
				// that caused problems when the framerate was above 190.
				if (dropFrame)
				{
					s_LastFrameTime = frameTime;
					return;
				}

				s_LastFrameTime = 0.0;
			}
			//else if (isPaused)
			//	// This hlClient virtual call just returns false.

			// Create and store usercmd structure.
			g_pHLClient->CreateMove(nextCommandNr, frameTime, !isPaused);
			cl->m_nOutgoingCommandNr = nextCommandNr;
		}

		CL_RunPrediction();

		if (sendPacket)
			CL_SendMove();
		else
			chan->SetChoked(); // Choke the packet...

		s_lastMovementCall = movementCallTime;
	}

	if (sendPacket)
	{
		if (isActive)
		{
			CLC_ClientTick tickMsg(cl->m_nDeltaTick, cl->m_nStringTableAckTick);
			chan->SendNetMsg(tickMsg, false, false);
		}

		chan->SendDatagram(nullptr);

		// Use full update rate when active.
		float delta = netTime - float(cl->m_flNextCmdTime);
		float maxDelta = fminf(fmaxf(delta, 0.0f), minFrameTime);

		cl->m_flNextCmdTime = double(minFrameTime + netTime - maxDelta);
	}
}

//-----------------------------------------------------------------------------
// Purpose: hook and run latency markers before simulation
//-----------------------------------------------------------------------------
void H_CL_Move()
{
	int currentReflexFrame = GFX_GetFrameNumber();

	if (currentReflexFrame != s_LastMovementReflexFrame)
		GFX_SetLatencyMarker(D3D11Device(), SIMULATION_START);

	CL_MoveEx();

	if (currentReflexFrame != s_LastMovementReflexFrame)
		GFX_SetLatencyMarker(D3D11Device(), SIMULATION_END);

	s_LastMovementReflexFrame = currentReflexFrame;
}

void VCL_Main::Attach() const
{
	DetourAttach(&CL_Move, &H_CL_Move);
}

void VCL_Main::Detach() const
{
	DetourDetach(&CL_Move, &H_CL_Move);
}
