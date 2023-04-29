#include "core/stdafx.h"
#include "tier0/threadtools.h"
#include "tier0/frametask.h"
#include "engine/server/sv_main.h"
#include "networksystem/pylon.h"
#include "networksystem/bansystem.h"
#include "engine/client/client.h"
#include "tier1/cvar.h"
#include "server.h"

//-----------------------------------------------------------------------------
// Purpose: checks if particular client is banned on the comp server
//-----------------------------------------------------------------------------
void SV_IsClientBanned(const string& svIPAddr, const uint64_t nNucleusID)
{
	string svError;

	bool bCompBanned = g_pMasterServer->CheckForBan(svIPAddr, nNucleusID, svError);
	if (bCompBanned)
	{
		if (!ThreadInMainThread())
		{
			g_TaskScheduler->Dispatch([svError, nNucleusID]
				{
					g_pBanSystem->AddConnectionRefuse(svError, nNucleusID); // Add to the vector.
				}, 0);
		}
		Warning(eDLL_T::SERVER, "Added '%s' to refused list ('%llu' is banned from the master server!)\n", svIPAddr.c_str(), nNucleusID);
	}
}

void SV_BroadcastVoiceData(CClient* cl, int nBytes, char* data)
{
	if (!sv_voiceenable->GetBool())
		return;

	if (g_ServerGlobalVariables->m_nMaxClients <= 0)
		return;

	SVC_VoiceData voiceData(cl->GetUserID(), nBytes, data);

	for (int i = 0; i < g_ServerGlobalVariables->m_nMaxClients; i++)
	{
		CClient* pClient = g_pClient->GetClient(i);

		if (!pClient)
			continue;

		// is this client fully connected
		if (pClient->GetSignonState() != SIGNONSTATE::SIGNONSTATE_FULL)
			continue;

		// is this client the sender
		if (pClient == cl && !sv_voiceEcho->GetBool())
			continue;

		// is this client on the sender's team
		if (pClient->GetTeamNum() != cl->GetTeamNum() && !sv_alltalk->GetBool())
			continue;

		// there's also supposed to be some xplat checks here
		// but since r5r is only on PC, there's no point in implementing them here

		CNetChan* pNetChan = pClient->GetNetChan();

		if (!pNetChan)
			continue;

		// if voice stream has enough space for new data
		if (pNetChan->GetStreamVoice().GetNumBitsLeft() >= 8 * nBytes + 96)
			pClient->SendNetMsg(&voiceData, false, false, true);
	}
}

///////////////////////////////////////////////////////////////////////////////

void HSV_Main::Attach(void) const
{
	DetourAttach(&v_SV_BroadcastVoiceData, SV_BroadcastVoiceData);
}

void HSV_Main::Detach(void) const
{
	DetourDetach(&v_SV_BroadcastVoiceData, SV_BroadcastVoiceData);
}

///////////////////////////////////////////////////////////////////////////////