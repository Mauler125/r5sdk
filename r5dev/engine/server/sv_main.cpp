//===========================================================================//
//
// Purpose:
//
//===========================================================================//
#include "core/stdafx.h"
#include "tier0/threadtools.h"
#include "tier0/frametask.h"
#include "tier1/cvar.h"
#include "engine/server/sv_main.h"
#include "engine/client/client.h"
#include "networksystem/pylon.h"
#include "networksystem/bansystem.h"
#include "engine/client/client.h"
#include "tier1/cvar.h"
#include "server.h"
#include "game/server/player.h"
#include "game/shared/util_shared.h"

//-----------------------------------------------------------------------------
// Purpose: checks if particular client is banned on the comp server
//-----------------------------------------------------------------------------
void SV_IsClientBanned(CClient* pClient, const string& svIPAddr,
	const uint64_t nNucleusID, const string& svPersonaName, const int nPort)
{
	Assert(pClient != nullptr);

	string svError;
	bool bCompBanned = g_pMasterServer->CheckForBan(svIPAddr, nNucleusID, svPersonaName, svError);

	if (bCompBanned)
	{
		if (!ThreadInMainThread())
		{
			g_TaskScheduler->Dispatch([pClient, svError, svIPAddr, nNucleusID, nPort]
				{
					// Make sure client isn't already disconnected,
					// and that if there is a valid netchannel, that
					// it hasn't been taken by a different client by
					// the time this task is getting executed.
					CNetChan* pChan = pClient->GetNetChan();
					if (pChan && pClient->GetNucleusID() == nNucleusID)
					{
						int nUserID = pClient->GetUserID();

						pClient->Disconnect(Reputation_t::REP_MARK_BAD, svError.c_str());
						Warning(eDLL_T::SERVER, "Removed client '[%s]:%i' from slot #%i ('%llu' is banned globally!)\n",
							svIPAddr.c_str(), nPort, nUserID, nNucleusID);
					}
				}, 0);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: checks if particular client is banned on the master server
//-----------------------------------------------------------------------------
void SV_ProcessBulkCheck(const BannedVec_t& bannedVec)
{
	BannedVec_t outBannedVec;
	g_pMasterServer->GetBannedList(bannedVec, outBannedVec);

	if (!ThreadInMainThread())
	{
		g_TaskScheduler->Dispatch([outBannedVec]
			{
				SV_CheckForBan(&outBannedVec);
			}, 0);
	}
}

//-----------------------------------------------------------------------------
// Purpose: creates a snapshot of the currently connected clients
// Input  : *pBannedVec - if passed, will check for bans and kick the clients
//-----------------------------------------------------------------------------
void SV_CheckForBan(const BannedVec_t* pBannedVec /*= nullptr*/)
{
	Assert(ThreadInMainThread());
	BannedVec_t bannedVec;

	for (int c = 0; c < g_ServerGlobalVariables->m_nMaxClients; c++) // Loop through all possible client instances.
	{
		CClient* pClient = g_pServer->GetClient(c);
		if (!pClient)
			continue;

		CNetChan* pNetChan = pClient->GetNetChan();
		if (!pNetChan)
			continue;

		if (!pClient->IsConnected())
			continue;

		if (pNetChan->GetRemoteAddress().IsLoopback())
			continue;

		const char* szIPAddr = pNetChan->GetAddress(true);
		const uint64_t nNucleusID = pClient->GetNucleusID();

		// If no banned list was provided, build one with all clients
		// on the server. This will be used for bulk checking so live
		// bans could be performed, as this function is called periodically.
		if (!pBannedVec)
			bannedVec.push_back(std::make_pair(string(szIPAddr), nNucleusID));
		else
		{
			// Check if current client is within provided banned list, and
			// prune if so...
			for (auto& it : *pBannedVec)
			{
				if (it.second == pClient->GetNucleusID())
				{
					const int nUserID = pClient->GetUserID();
					const int nPort = pNetChan->GetPort();

					pClient->Disconnect(Reputation_t::REP_MARK_BAD, "%s", it.first.c_str());
					Warning(eDLL_T::SERVER, "Removed client '[%s]:%i' from slot #%i ('%llu' is banned globally!)\n",
						szIPAddr, nPort, nUserID, nNucleusID);
				}
			}
		}
	}

	if (!pBannedVec && !bannedVec.empty())
	{
		std::thread(&SV_ProcessBulkCheck, bannedVec).detach();
	}
}

//-----------------------------------------------------------------------------
// Purpose: loads the game .dll
//-----------------------------------------------------------------------------
void SV_InitGameDLL()
{
	v_SV_InitGameDLL();
}

//-----------------------------------------------------------------------------
// Purpose: release resources associated with extension DLLs.
//-----------------------------------------------------------------------------
void SV_ShutdownGameDLL()
{
	v_SV_ShutdownGameDLL();
}

//-----------------------------------------------------------------------------
// Purpose: activates the server
// Output : true on success, false on failure
//-----------------------------------------------------------------------------
bool SV_ActivateServer()
{
	return v_SV_ActivateServer();
}

void SV_BroadcastVoiceData(CClient* cl, int nBytes, char* data)
{
	if (!sv_voiceenable->GetBool())
		return;

	if (g_ServerGlobalVariables->m_nMaxClients <= 0)
		return;

	SVC_VoiceData voiceData(cl->GetUserID(), nBytes, data);
	CBaseEntity* cEnt = UTIL_PlayerByIndex(cl->GetHandle());
	Vector3D sourcePlayerOrigin = cEnt->GetAbsOrigin();

	for (int i = 0; i < g_ServerGlobalVariables->m_nMaxClients; i++)
	{
		CClient* pClient = g_pServer->GetClient(i);

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

		// is this client too far
		CBaseEntity* pEnt = UTIL_PlayerByIndex(pClient->GetHandle());
		Vector3D destPlayerOrigin = pEnt->GetAbsOrigin();
		Vector3D vecDist = destPlayerOrigin - sourcePlayerOrigin;

		if (vecDist.Length() > 2625 && sv_alltalk->GetBool()) // 50 m
			continue;

		// if voice stream has enough space for new data
		if (pNetChan->GetStreamVoice().GetNumBitsLeft() >= 8 * nBytes + 96)
			pClient->SendNetMsgEx(&voiceData, false, false, true);
	}
}

///////////////////////////////////////////////////////////////////////////////
void HSV_Main::Attach(void) const
{
	//DetourAttach(&v_SV_InitGameDLL, SV_InitGameDLL);
	//DetourAttach(&v_SV_ShutdownGameDLL, SV_ShutdownGameDLL);
	//DetourAttach(&v_SV_ActivateServer, SV_ActivateServer);
	DetourAttach(&v_SV_BroadcastVoiceData, SV_BroadcastVoiceData);
}

void HSV_Main::Detach(void) const
{
	//DetourDetach(&v_SV_InitGameDLL, SV_InitGameDLL);
	//DetourDetach(&v_SV_ShutdownGameDLL, SV_ShutdownGameDLL);
	//DetourDetach(&v_SV_ActivateServer, SV_ActivateServer);
	DetourDetach(&v_SV_BroadcastVoiceData, SV_BroadcastVoiceData);
}
