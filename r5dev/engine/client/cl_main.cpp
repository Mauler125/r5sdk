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

//-----------------------------------------------------------------------------
// Purpose: run client's movement frame
//-----------------------------------------------------------------------------
void CL_MoveEx()
{
	CClientState* const cl = GetBaseLocalClient();

	if (!cl->IsConnected())
		return;

	if (!v_Host_ShouldRun())
		return;

	const int commandTick = cl->m_CurrFrameSnapshot
		? cl->m_CurrFrameSnapshot->m_TickUpdate.m_nCommandTick
		: -1;

	bool sendPacket = true;
	CNetChan* const chan = cl->m_NetChannel;

	// Only perform clamping and packeting if the timescale value is default,
	// else the timescale change won't be handled in the player's movement.
	const float hostTimeScale = host_timescale->GetFloat();
	const bool isTimeScaleDefault = hostTimeScale == 1.0;

	const float minFrameTime = usercmd_frametime_min.GetFloat();
	const float maxFrameTime = usercmd_frametime_max.GetFloat();

	const float netTime = float(*g_pNetTime);

	if (cl->m_flNextCmdTime <= (maxFrameTime * 0.5f) + netTime)
		sendPacket = chan->CanPacket();

	else if (cl->m_nOutgoingCommandNr - (commandTick+1) < MAX_NEW_COMMANDS || isTimeScaleDefault)
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

			// Create and store usercmd structure.
			g_pHLClient->CreateMove(nextCommandNr, frameTime, !isPaused);
			cl->m_nOutgoingCommandNr = nextCommandNr;
		}

		v_CL_RunPrediction();

		if (sendPacket)
			v_CL_SendMove();
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
		const float delta = netTime - float(cl->m_flNextCmdTime);
		const float maxDelta = fminf(fmaxf(delta, 0.0f), minFrameTime);

		cl->m_flNextCmdTime = double(minFrameTime + netTime - maxDelta);
	}
}

void VCL_Main::Detour(const bool bAttach) const
{
	DetourSetup(&v_CL_Move, &CL_MoveEx, bAttach);
}
