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
// Input  : *this - 
//			*pInpacket - 
// Output : pointer to client instance on success, nullptr on failure
//---------------------------------------------------------------------------------
CClient* CServer::Authenticate(CServer* pServer, user_creds_s* pInpacket)
{
	string svIpAddress = pInpacket->m_nAddr.GetAddress();
	if (sv_showconnecting->GetBool())
	{
		DevMsg(eDLL_T::SERVER, "Processing connectionless challenge from '%s' ('%llu')\n", svIpAddress.c_str(), pInpacket->m_nNucleusID);
	}

	if (g_pBanSystem->IsBanListValid()) // Is the banlist vector valid?
	{
		if (g_pBanSystem->IsBanned(svIpAddress, pInpacket->m_nNucleusID)) // Is the client trying to connect banned?
		{
			v_CServer_RejectConnection(pServer, pServer->m_Socket, pInpacket, "You have been banned from this server."); // RejectConnection for the client.
			if (sv_showconnecting->GetBool())
			{
				Warning(eDLL_T::SERVER, "Connection rejected for '%s' ('%llu' is banned from this server!)\n", svIpAddress.c_str(), pInpacket->m_nNucleusID);
			}

			return nullptr;
		}
	}

	if (g_bCheckCompBanDB)
	{
		std::thread th(SV_IsClientBanned, g_pMasterServer, svIpAddress, pInpacket->m_nNucleusID);
		th.detach();
	}

	return v_CServer_Authenticate(pServer, pInpacket);
}

///////////////////////////////////////////////////////////////////////////////
void CServer_Attach()
{
	DetourAttach((LPVOID*)&v_CServer_Authenticate, &CServer::Authenticate);
}

void CServer_Detach()
{
	DetourDetach((LPVOID*)&v_CServer_Authenticate, &CServer::Authenticate);
}

///////////////////////////////////////////////////////////////////////////////
bool g_bCheckCompBanDB = true; // Maybe make this a static method in CServer? It won't be added to the struct offsets then.
CServer* g_pServer = nullptr;