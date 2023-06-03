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

static float s_lastMovementCall = 0.0;
static float s_LastFrameTime = 0.0;

//-----------------------------------------------------------------------------
// Purpose: run client's movement frame
//-----------------------------------------------------------------------------
void H_CL_Move()
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
	float netTime = float(*g_pNetTime);
	CNetChan* chan = cl->m_NetChannel;

	if (cl->m_flNextCmdTime <= (0.5 / cl_cmdrate->GetFloat()) + netTime)
		sendPacket = chan->CanPacket();

	else if (g_pClientState->m_nOutgoingCommandNr - (commandTick+1) < 15 || host_timescale->GetFloat() == 1.0)
		sendPacket = false;

	if (cl->IsActive())
	{
		float timeNow = float(Plat_FloatTime());
		int outCommandNr = g_pClientState->m_nOutgoingCommandNr;

		bool isPaused = g_pClientState->IsPaused();
		int nextCommandNr = isPaused ? outCommandNr : outCommandNr+1;

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
					frameTime = timeNow - s_lastMovementCall;
					deltaTime = frameTime;
				}
				else
				{
					timeScale = host_timescale->GetFloat();
					frameTime = cl->m_flFrameTime + s_LastFrameTime;
					deltaTime = frameTime / timeScale;
				}

				if (deltaTime > 0.1f)
					frameTime = timeScale * 0.1f;

				// This check originally was 'time < 0.0049999999', but
				// that caused problems when the framerate was above 190.
				if ((1.0 / fps_input_max->GetFloat()) > deltaTime)
				{
					s_LastFrameTime = frameTime;
					return;
				}

				s_LastFrameTime = 0.0;
			}
			//else if (isPaused)
			//{
			//	// This hlClient virtual call just returns false.
			//}

			// Create and store usercmd structure.
			g_pHLClient->CreateMove(nextCommandNr, frameTime, !isPaused);
			g_pClientState->m_nOutgoingCommandNr = nextCommandNr;
		}

		CL_RunPrediction();

		if (sendPacket)
		{
			CL_SendMove();

			CLC_ClientTick tickMsg(cl->m_nDeltaTick, cl->m_nStringTableAckTick);

			chan->SendNetMsg(tickMsg, false, false);
			chan->SendDatagram(nullptr);

			// Use full update rate when active.
			float delta = netTime - float(g_pClientState->m_flNextCmdTime);
			float commandInterval = (1.0f / cl_cmdrate->GetFloat()) - 0.001f;
			float maxDelta = 0.0f;

			if (delta >= 0.0f)
				maxDelta = fminf(commandInterval, delta);

			g_pClientState->m_flNextCmdTime = double(commandInterval + netTime - maxDelta);
		}
		else // Choke the packet...
			chan->SetChoked();

		s_lastMovementCall = timeNow;
	}
}

void VCL_Main::Attach() const
{
	DetourAttach(&CL_Move, &H_CL_Move);
}

void VCL_Main::Detach() const
{
	DetourDetach(&CL_Move, &H_CL_Move);
}
