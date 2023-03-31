//=============================================================================//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// server.cpp: implementation of the CServer class.
//
/////////////////////////////////////////////////////////////////////////////////
#include "core/stdafx.h"
#include "common/protocol.h"
#include "tier1/cvar.h"
#include "engine/server/sv_main.h"
#include "engine/server/server.h"
#include "networksystem/pylon.h"
#include "networksystem/bansystem.h"
#include "ebisusdk/EbisuSDK.h"
#include "public/edict.h"

//---------------------------------------------------------------------------------
// Purpose: Gets the number of human players on the server
// Output : int
//---------------------------------------------------------------------------------
int CServer::GetNumHumanPlayers(void) const
{
	int nHumans = 0;
	for (int i = 0; i < g_ServerGlobalVariables->m_nMaxClients; i++)
	{
		CClient* pClient = g_pClient->GetClient(i);
		if (!pClient)
			continue;

		if (pClient->IsHumanPlayer())
			nHumans++;
	}

	return nHumans;
}

//---------------------------------------------------------------------------------
// Purpose: Gets the number of fake clients on the server
// Output : int
//---------------------------------------------------------------------------------
int CServer::GetNumFakeClients(void) const
{
	int nBots = 0;
	for (int i = 0; i < g_ServerGlobalVariables->m_nMaxClients; i++)
	{
		CClient* pClient = g_pClient->GetClient(i);
		if (!pClient)
			continue;

		if (pClient->IsConnected() && pClient->IsFakeClient())
			nBots++;
	}

	return nBots;
}

//---------------------------------------------------------------------------------
// Purpose: Gets the number of clients on the server
// Output : int
//---------------------------------------------------------------------------------
int CServer::GetNumClients(void) const
{
	int nClients = 0;
	for (int i = 0; i < g_ServerGlobalVariables->m_nMaxClients; i++)
	{
		CClient* pClient = g_pClient->GetClient(i);
		if (!pClient)
			continue;

		if (pClient->IsConnected())
			nClients++;
	}

	return nClients;
}

//---------------------------------------------------------------------------------
// Purpose: client to server authentication
// Input  : *pChallenge - 
// Output : true if user isn't banned, false otherwise
//---------------------------------------------------------------------------------
bool CServer::AuthClient(user_creds_s* pChallenge)
{
	char* pUserID = pChallenge->m_pUserID;
	uint64_t nNucleusID = pChallenge->m_nNucleusID;

	char pszAddresBuffer[INET6_ADDRSTRLEN]; // Render the client's address.
	pChallenge->m_nAddr.ToString(pszAddresBuffer, sizeof(pszAddresBuffer));

	const bool bEnableLogging = sv_showconnecting->GetBool();
	if (bEnableLogging)
		DevMsg(eDLL_T::SERVER, "Processing connectionless challenge for '%s' ('%llu')\n", pszAddresBuffer, nNucleusID);

	// Only proceed connection if the client's name is valid and UTF-8 encoded.
	if (!pUserID || !pUserID[0] || !IsValidUTF8(pUserID) || !IsValidPersonaName(pUserID))
	{
		RejectConnection(m_Socket, &pChallenge->m_nAddr, "#Valve_Reject_Invalid_Name");
		if (bEnableLogging)
			Warning(eDLL_T::SERVER, "Connection rejected for '%s' ('%llu' has an invalid name!)\n", pszAddresBuffer, nNucleusID);

		return false;
	}

	if (g_pBanSystem->IsBanListValid())
	{
		if (g_pBanSystem->IsBanned(pszAddresBuffer, nNucleusID))
		{
			RejectConnection(m_Socket, &pChallenge->m_nAddr, "#Valve_Reject_Banned");
			if (bEnableLogging)
				Warning(eDLL_T::SERVER, "Connection rejected for '%s' ('%llu' is banned from this server!)\n", pszAddresBuffer, nNucleusID);

			return false;
		}
	}

	if (sv_globalBanlist->GetBool())
	{
		std::thread th(SV_IsClientBanned, string(pszAddresBuffer), nNucleusID);
		th.detach();
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Initializes a CSVClient for a new net connection. This will only be called
//			once for a player each game, not once for each level change.
// Input  : *pServer - 
//			*pInpacket - 
// Output : pointer to client instance on success, nullptr on failure
//---------------------------------------------------------------------------------
CClient* CServer::ConnectClient(CServer* pServer, user_creds_s* pChallenge)
{
	if (pServer->m_State < server_state_t::ss_active || !pServer->AuthClient(pChallenge))
		return nullptr;

	CClient* pClient = v_CServer_ConnectClient(pServer, pChallenge);
	return pClient;
}

//---------------------------------------------------------------------------------
// Purpose: Rejects connection request and sends back a message
// Input  : iSocket - 
//			*pChallenge - 
//			*szMessage - 
//---------------------------------------------------------------------------------
void CServer::RejectConnection(int iSocket, netadr_t* pNetAdr, const char* szMessage)
{
	v_CServer_RejectConnection(this, iSocket, pNetAdr, szMessage);
}

//---------------------------------------------------------------------------------
// Purpose: Runs the server frame job
// Input  : flFrameTime - 
//			bRunOverlays - 
//			bUniformSnapshotInterval - 
//---------------------------------------------------------------------------------
void CServer::FrameJob(double flFrameTime, bool bRunOverlays, bool bUniformSnapshotInterval)
{
	v_CServer_FrameJob(flFrameTime, bRunOverlays, bUniformSnapshotInterval);
}

//---------------------------------------------------------------------------------
// Purpose: Runs the server frame
// Input  : *pServer - 
//---------------------------------------------------------------------------------
void CServer::RunFrame(CServer* pServer)
{
	v_CServer_RunFrame(pServer);
}

///////////////////////////////////////////////////////////////////////////////
void VServer::Attach() const
{
	DetourAttach(&v_CServer_RunFrame, &CServer::RunFrame);
#if	defined(GAMEDLL_S3)
	DetourAttach((LPVOID*)&v_CServer_ConnectClient, &CServer::ConnectClient);
	DetourAttach((LPVOID*)&v_CServer_FrameJob, &CServer::FrameJob);
#endif // !TODO: S1 and S2 CServer functions require work.
}

void VServer::Detach() const
{
	DetourDetach(&v_CServer_RunFrame, &CServer::RunFrame);
#if	defined(GAMEDLL_S3)
	DetourDetach((LPVOID*)&v_CServer_ConnectClient, &CServer::ConnectClient);
	DetourDetach((LPVOID*)&v_CServer_FrameJob, &CServer::FrameJob);
#endif // !TODO: S1 and S2 CServer functions require work.
}

///////////////////////////////////////////////////////////////////////////////
CServer* g_pServer = nullptr;