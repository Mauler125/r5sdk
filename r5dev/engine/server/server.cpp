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
// Purpose: client to server authentication
// Input  : *pChallenge - 
// Output : true if user isn't banned, false otherwise
//---------------------------------------------------------------------------------
bool CServer::AuthClient(user_creds_s* pChallenge)
{
	string svIpAddress = pChallenge->m_nAddr.GetAddress();
	if (sv_showconnecting->GetBool())
		DevMsg(eDLL_T::SERVER, "Processing connectionless challenge for '%s' ('%llu')\n", svIpAddress.c_str(), pChallenge->m_nNucleusID);

	if (g_pBanSystem->IsBanListValid()) // Is the banned list vector valid?
	{
		if (g_pBanSystem->IsBanned(svIpAddress, pChallenge->m_nNucleusID)) // Is the client trying to connect banned?
		{
			RejectConnection(m_Socket, pChallenge, "#Valve_Reject_Banned"); // RejectConnection for the client.
			if (sv_showconnecting->GetBool())
				Warning(eDLL_T::SERVER, "Connection rejected for '%s' ('%llu' is banned from this server!)\n", svIpAddress.c_str(), pChallenge->m_nNucleusID);

			return false;
		}
	}

	if (g_bCheckCompBanDB)
	{
		std::thread th(SV_IsClientBanned, svIpAddress, pChallenge->m_nNucleusID);
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
	if (pServer->AuthClient(pChallenge))
	{
		CClient* pClient = v_CServer_ConnectClient(pServer, pChallenge);
		return pClient;
	}

	return nullptr;
}

//---------------------------------------------------------------------------------
// Purpose: Rejects connection request and sends back a message
// Input  : *iSocket - 
//			*pChallenge - 
//			*szMessage - 
//---------------------------------------------------------------------------------
void CServer::RejectConnection(int iSocket, user_creds_s* pChallenge, const char* szMessage)
{
	v_CServer_RejectConnection(this, iSocket, pChallenge, szMessage);
}

///////////////////////////////////////////////////////////////////////////////
void CServer_Attach()
{
	DetourAttach((LPVOID*)&v_CServer_ConnectClient, &CServer::ConnectClient);
}

void CServer_Detach()
{
	DetourDetach((LPVOID*)&v_CServer_ConnectClient, &CServer::ConnectClient);
}

///////////////////////////////////////////////////////////////////////////////
bool g_bCheckCompBanDB = true; // Maybe make this a static method in CServer? It won't be added to the struct offsets then.
CServer* g_pServer = nullptr;