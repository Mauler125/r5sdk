//===========================================================================//
//
// Purpose: Implementation of the rcon server
//
//===========================================================================//

#include "core/stdafx.h"
#include "tier0/cmd.h"
#include "tier0/cvar.h"
#include "tier0/IConVar.h"
#include "tier1/NetAdr2.h"
#include "tier2/socketcreator.h"
#include "engine/sys_utils.h"
#include "engine/sv_rcon.h"
#include "mathlib/sha256.h"
#include "client/IVEngineClient.h"
#include "common/igameserverdata.h"

//-----------------------------------------------------------------------------
// Purpose: creates listen socket for RCON
//-----------------------------------------------------------------------------
void CRConServer::Init(void)
{
	if (std::strlen(rcon_password->GetString()) < 8)
	{
		if (std::strlen(rcon_password->GetString()) != 0)
		{
			DevMsg(eDLL_T::SERVER, "Remote server access requires a password of at least 8 characters\n");
		}

		if (m_pSocket->IsListening())
		{
			m_pSocket->CloseListenSocket();
		}
		m_bInitialized = false;
		return;
	}

	static ConVar* hostport = g_pCVar->FindVar("hostport");

	m_pAdr2 = new CNetAdr2(rcon_address->GetString(), hostport->GetString());
	m_pSocket->CreateListenSocket(*m_pAdr2, false);

	DevMsg(eDLL_T::SERVER, "Remote server access initialized\n");
	m_bInitialized = true;
}

//-----------------------------------------------------------------------------
// Purpose: run tasks for the RCON server
//-----------------------------------------------------------------------------
void CRConServer::Think(void)
{
	int nCount = m_pSocket->GetAcceptedSocketCount();

	// Close redundant sockets if there are too many except for whitelisted and authenticated.
	if (nCount >= sv_rcon_maxsockets->GetInt())
	{
		for (m_nConnIndex = nCount - 1; m_nConnIndex >= 0; m_nConnIndex--)
		{
			CNetAdr2 netAdr2 = m_pSocket->GetAcceptedSocketAddress(m_nConnIndex);
			if (std::strcmp(netAdr2.GetIP(true).c_str(), sv_rcon_whitelist_address->GetString()) != 0)
			{
				CConnectedNetConsoleData* pData = m_pSocket->GetAcceptedSocketData(m_nConnIndex);
				if (!pData->m_bAuthorized)
				{
					CloseConnection();
				}
			}
		}
	}

	// Create a new listen socket if authenticated socket is closed.
	if (nCount == 0)
	{
		if (!m_pSocket->IsListening())
		{
			m_pSocket->CreateListenSocket(*m_pAdr2, false);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: server RCON main loop (run this every frame)
//-----------------------------------------------------------------------------
void CRConServer::RunFrame(void)
{
	if (m_bInitialized)
	{
		m_pSocket->RunFrame();
		this->Think();
		this->Recv();
	}
}

//-----------------------------------------------------------------------------
// Purpose: process outgoing packet
// Input  : *pszBuf - 
//-----------------------------------------------------------------------------
void CRConServer::Send(const char* pszBuf)
{
	int nCount = m_pSocket->GetAcceptedSocketCount();

	for (int i = nCount - 1; i >= 0; i--)
	{
		CConnectedNetConsoleData* pData = m_pSocket->GetAcceptedSocketData(i);

		if (pData->m_bAuthorized)
		{
			::send(pData->m_hSocket, pszBuf, strlen(pszBuf), MSG_NOSIGNAL);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: process incoming packet
//-----------------------------------------------------------------------------
void CRConServer::Recv(void)
{
	int nCount = m_pSocket->GetAcceptedSocketCount();

	for (m_nConnIndex = nCount - 1; m_nConnIndex >= 0; m_nConnIndex--)
	{
		CConnectedNetConsoleData* pData = m_pSocket->GetAcceptedSocketData(m_nConnIndex);

		{//////////////////////////////////////////////
			if (CheckForBan(pData))
			{
				::send(pData->m_hSocket, s_pszBannedMessage, strlen(s_pszBannedMessage), MSG_NOSIGNAL);
				CloseConnection();
				continue;
			}

			char szRecvBuf{};
			int nPendingLen = ::recv(pData->m_hSocket, &szRecvBuf, sizeof(szRecvBuf), MSG_PEEK);

			if (nPendingLen == SOCKET_ERROR && m_pSocket->IsSocketBlocking())
			{
				continue;
			}

			if (nPendingLen <= 0) // EOF or error.
			{
				CloseConnection();
				continue;
			}
		}//////////////////////////////////////////////

		u_long nReadLen; // Find out how much we have to read.
		::ioctlsocket(pData->m_hSocket, FIONREAD, &nReadLen);

		while (nReadLen > 0 && nReadLen < MAX_NETCONSOLE_INPUT_LEN -1)
		{
			char szRecvBuf[MAX_NETCONSOLE_INPUT_LEN]{};
			int nRecvLen = ::recv(pData->m_hSocket, szRecvBuf, MIN(sizeof(szRecvBuf), nReadLen), 0);

			if (nRecvLen == 0) // Socket was closed.
			{
				CloseConnection();
				break;
			}

			if (nRecvLen < 0 && !m_pSocket->IsSocketBlocking())
			{
				break;
			}

			nReadLen -= nRecvLen;

			// Write what we've got into the command buffer.
			HandleInputChars(szRecvBuf, nRecvLen, pData);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: authenticate new connections
// Input  : *pData - 
// TODO   : implement logic for key exchange instead so we never network our
// password in plain text over the wire. create a cvar for this so user could
// also opt out and use legacy authentication instead for older RCON clients
//-----------------------------------------------------------------------------
void CRConServer::Auth(CConnectedNetConsoleData* pData)
{
	if (pData->m_bAuthorized)
	{
		return;
	}
	else if (std::memcmp(pData->m_pszInputCommandBuffer, "PASS ", 5) == 0)
	{
		if (std::strcmp(pData->m_pszInputCommandBuffer + 5, rcon_password->GetString()) == 0)
		{ // TODO: Hash and compare password with SHA256 instead!
			pData->m_bAuthorized = true;
			m_pSocket->CloseListenSocket();
			this->CloseNonAuthConnection();
		}
		else // Bad password.
		{
			CNetAdr2 netAdr2 = m_pSocket->GetAcceptedSocketAddress(m_nConnIndex);
			DevMsg(eDLL_T::SERVER, "Bad RCON password attempt from '%s'\n", netAdr2.GetIPAndPort().c_str());
			::send(pData->m_hSocket, s_pszWrongPwMessage, strlen(s_pszWrongPwMessage), MSG_NOSIGNAL);

			pData->m_bAuthorized = false;
			pData->m_nFailedAttempts++;
		}
	}
	else
	{
		::send(pData->m_hSocket, s_pszNoAuthMessage, strlen(s_pszNoAuthMessage), MSG_NOSIGNAL);
		pData->m_nIgnoredMessage++;
	}
}

//-----------------------------------------------------------------------------
// Purpose: handles input command buffer
// Input  : *pszIn - 
//			nRecvLen - 
//			*pData - 
//-----------------------------------------------------------------------------
void CRConServer::HandleInputChars(const char* pszIn, int nRecvLen, CConnectedNetConsoleData* pData)
{
	while (nRecvLen)
	{
		switch (*pszIn)
		{
		case '\r':
		case '\n':
		{
			if (pData->m_nCharsInCommandBuffer)
			{
				pData->m_pszInputCommandBuffer[pData->m_nCharsInCommandBuffer] = 0;
				this->Auth(pData);

				// Only execute if auth was succesfull.
				if (pData->m_bAuthorized)
				{
					this->Execute(pData);
				}
			}
			pData->m_nCharsInCommandBuffer = 0;
			break;
		}
		default:
		{
			if (pData->m_nCharsInCommandBuffer < MAX_NETCONSOLE_INPUT_LEN - 1)
			{
				pData->m_pszInputCommandBuffer[pData->m_nCharsInCommandBuffer++] = *pszIn;
			}
			break;
		}
		}
		pszIn++;
		nRecvLen--;
	}
}

//-----------------------------------------------------------------------------
// Purpose: execute commands issued from net console
// Input  : *pData - 
//-----------------------------------------------------------------------------
void CRConServer::Execute(CConnectedNetConsoleData* pData)
{
	IVEngineClient_CommandExecute(NULL, pData->m_pszInputCommandBuffer);
}

//-----------------------------------------------------------------------------
// Purpose: checks for amount of failed attempts and bans net console accordingly
// Input  : *pData - 
//-----------------------------------------------------------------------------
bool CRConServer::CheckForBan(CConnectedNetConsoleData* pData)
{
	CNetAdr2 netAdr2 = m_pSocket->GetAcceptedSocketAddress(m_nConnIndex);

	// Check if IP is in the ban vector.
	if (std::find(m_vBannedAddress.begin(), m_vBannedAddress.end(),
		netAdr2.GetIP(true)) != m_vBannedAddress.end())
	{
		return true;
	}

	// Check if net console has reached maximum number of attempts and add to ban vector.
	if (pData->m_nFailedAttempts >= sv_rcon_maxfailures->GetInt()
		|| pData->m_nIgnoredMessage >= sv_rcon_maxignores->GetInt())
	{
		// Don't add whitelisted address to ban vector.
		if (std::strcmp(netAdr2.GetIP(true).c_str(), sv_rcon_whitelist_address->GetString()) == 0)
		{
			pData->m_nFailedAttempts = 0;
			pData->m_nIgnoredMessage = 0;
			return false;
		}

		DevMsg(eDLL_T::SERVER, "Banned '%s' for RCON hacking attempts\n", netAdr2.GetIPAndPort().c_str());
		m_vBannedAddress.push_back(netAdr2.GetIP(true));
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: close specific connection
//-----------------------------------------------------------------------------
void CRConServer::CloseConnection(void) // NETMGR
{
	m_pSocket->CloseAcceptedSocket(m_nConnIndex);
}

//-----------------------------------------------------------------------------
// Purpose: close all connections except for authenticated
//-----------------------------------------------------------------------------
void CRConServer::CloseNonAuthConnection(void)
{
	int nCount = m_pSocket->GetAcceptedSocketCount();

	for (int i = nCount - 1; i >= 0; i--)
	{
		CConnectedNetConsoleData* pData = m_pSocket->GetAcceptedSocketData(i);

		if (!pData->m_bAuthorized)
		{
			m_pSocket->CloseAcceptedSocket(i);
		}
	}
}
///////////////////////////////////////////////////////////////////////////////
CRConServer* g_pRConServer = new CRConServer();
