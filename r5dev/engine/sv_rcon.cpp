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
// Purpose: Create's listen socket for RCON
//-----------------------------------------------------------------------------
void CRConServer::Init(void)
{
	if (std::strlen(rcon_password->GetString()) < 8)
	{
		DevMsg(eDLL_T::SERVER, "RCON disabled\n");

		m_bInitialized = false;
		return;
	}

	static ConVar* hostport = g_pCVar->FindVar("hostport");
	m_pAdr2 = new CNetAdr2(rcon_address->GetString(), hostport->GetString());
	m_pSocket->CreateListenSocket(*m_pAdr2, false);

	m_bInitialized = true;
}

//-----------------------------------------------------------------------------
// Purpose: run tasks for RCON
//-----------------------------------------------------------------------------
void CRConServer::Think(void)
{
}

//-----------------------------------------------------------------------------
// Purpose: server RCON main loop (run this every frame)
//-----------------------------------------------------------------------------
void CRConServer::RunFrame(void)
{
	if (m_bInitialized)
	{
		m_pSocket->RunFrame();
		ProcessMessage();
	}
}

//-----------------------------------------------------------------------------
// Purpose: process incoming packet
//-----------------------------------------------------------------------------
void CRConServer::ProcessMessage(void)
{
	int nCount = m_pSocket->GetAcceptedSocketCount();

	for (int i = nCount - 1; i >= 0; i--)
	{
		CConnectedNetConsoleData* pData = m_pSocket->GetAcceptedSocketData(i);

		{//////////////////////////////////////////////
			if (CheckForBan(i, pData))
			{
				send(pData->m_hSocket, s_pszBannedMessage, strlen(s_pszBannedMessage), MSG_NOSIGNAL);
				CloseConnection(i);
				continue;
			}

			char szRecvBuf{};
			int nPendingLen = recv(pData->m_hSocket, &szRecvBuf, sizeof(szRecvBuf), MSG_PEEK);

			if (nPendingLen == SOCKET_ERROR && m_pSocket->IsSocketBlocking())
			{
				continue;
			}

			if (nPendingLen <= 0) // EOF or error.
			{
				CloseConnection(i);
				continue;
			}
		}//////////////////////////////////////////////

		u_long nReadLen; // Find out how much we have to read.
		ioctlsocket(pData->m_hSocket, FIONREAD, &nReadLen);

		while (nReadLen > 0 && nReadLen < MAX_NETCONSOLE_INPUT_LEN -1)
		{
			char szRecvBuf[MAX_NETCONSOLE_INPUT_LEN]{};
			int nRecvLen = ::recv(pData->m_hSocket, szRecvBuf, MIN(sizeof(szRecvBuf), nReadLen), 0);

			if (nRecvLen == 0) // Socket was closed.
			{
				CloseConnection(i);
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
// TODO   : implement logic for key exchange instead so we never network our
// password in plain text over the wire. create a cvar for this so user could
// also opt out and use legacy authentication instead for legacy RCON clients
//-----------------------------------------------------------------------------
void CRConServer::Authenticate(CConnectedNetConsoleData* pData)
{
	if (pData->m_bAuthorized)
	{
		return;
	}
	else if (std::memcmp(pData->m_pszInputCommandBuffer, "PASS ", 5) == 0)
	{
		if (std::strcmp(pData->m_pszInputCommandBuffer + 5, rcon_password->GetString()) == 0)
		{ // TODO: Hash password instead!
			pData->m_bAuthorized = true;
		}
		else // Bad password.
		{
			DevMsg(eDLL_T::SERVER, "Bad password attempt from net console\n");
			::send(pData->m_hSocket, s_pszWrongPwMessage, strlen(s_pszWrongPwMessage), MSG_NOSIGNAL);

			pData->m_bAuthorized = false;
			pData->m_nFailedAttempts++;
		}
	}
	else
	{
		::send(pData->m_hSocket, s_pszNoAuthMessage, strlen(s_pszNoAuthMessage), MSG_NOSIGNAL);
	}
}

//-----------------------------------------------------------------------------
// Purpose: handles input command buffer
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
				Authenticate(pData);

				if (pData->m_bAuthorized)
				{
					Execute(pData);
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
// Purpose: 
//-----------------------------------------------------------------------------
void CRConServer::Execute(CConnectedNetConsoleData* pData)
{
	IVEngineClient_CommandExecute(NULL, pData->m_pszInputCommandBuffer);
}

//-----------------------------------------------------------------------------
// Purpose: checks for amount of failed attempts and bans netconsole accordingly
//-----------------------------------------------------------------------------
bool CRConServer::CheckForBan(int nIdx, CConnectedNetConsoleData* pData)
{
	CNetAdr2 netAdr2 = m_pSocket->GetAcceptedSocketAddress(nIdx);

	// Check if IP is in the ban vector.
	if (std::find(m_vBannedAddress.begin(), m_vBannedAddress.end(), 
		netAdr2.GetIP(true)) != m_vBannedAddress.end())
	{
		return true;
	}

	// Check if netconsole has reached maximum number of attempts and add to ban vector.
	if (pData->m_nFailedAttempts >= sv_rcon_maxfailures->GetInt())
	{
		m_vBannedAddress.push_back(netAdr2.GetIP(true));
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRConServer::CloseConnection(int nIdx) // NETMGR
{
	m_pSocket->CloseAcceptedSocket(nIdx);
}

CRConServer* g_pRConServer = nullptr;
