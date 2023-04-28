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
			g_TaskScheduler->Dispatch([svError, svIPAddr]
				{
					g_pBanSystem->KickPlayerById(svIPAddr.c_str(), svError.c_str());
				}, 0);
		}

		//Warning(eDLL_T::SERVER, "Added '%s' to refused list ('%llu' is banned from the master server!)\n", svIPAddr.c_str(), nNucleusID);
	}
}

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

void SV_CheckForBan(const BannedVec_t* pBannedVec /*= nullptr*/)
{
	BannedVec_t bannedVec;

	for (int c = 0; c < MAX_PLAYERS; c++) // Loop through all possible client instances.
	{
		CClient* pClient = g_pClient->GetClient(c);
		if (!pClient)
			continue;

		CNetChan* pNetChan = pClient->GetNetChan();
		if (!pNetChan)
			continue;

		if (!pClient->IsConnected())
			continue;

		const char* szIPAddr = pNetChan->GetAddress();
		const uint64_t nNucleusID = pClient->GetNucleusID();

		if (!pBannedVec)
			bannedVec.push_back(std::make_pair(szIPAddr, nNucleusID));
		else
		{
			for (auto& it : *pBannedVec)
			{
				if (it.second == pClient->GetNucleusID())
				{
					Warning(eDLL_T::SERVER, "Removing client '%s' from slot '%i' ('%llu' is banned from this server!)\n", szIPAddr, c, nNucleusID);
					pClient->Disconnect(Reputation_t::REP_MARK_BAD, "%s", it.first.c_str());
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

///////////////////////////////////////////////////////////////////////////////

void HSV_Main::Attach() const
{
	//DetourAttach(&v_SV_InitGameDLL, SV_InitGameDLL);
	//DetourAttach(&v_SV_ShutdownGameDLL, SV_ShutdownGameDLL);
	//DetourAttach(&v_SV_ActivateServer, SV_ActivateServer);
}

void HSV_Main::Detach() const
{
	//DetourDetach(&v_SV_InitGameDLL, SV_InitGameDLL);
	//DetourDetach(&v_SV_ShutdownGameDLL, SV_ShutdownGameDLL);
	//DetourDetach(&v_SV_ActivateServer, SV_ActivateServer);
}