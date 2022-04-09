//===========================================================================//
// 
// Purpose: Implementation of the rcon server.
// 
//===========================================================================//

#include "core/stdafx.h"
#include "tier1/cmd.h"
#include "tier1/cvar.h"
#include "tier1/IConVar.h"
#include "tier1/NetAdr2.h"
#include "tier2/socketcreator.h"
#include "engine/sys_utils.h"
#include "engine/sv_rcon.h"
#include "protoc/sv_rcon.pb.h"
#include "protoc/cl_rcon.pb.h"
#include "mathlib/sha256.h"
#include "common/igameserverdata.h"

//-----------------------------------------------------------------------------
// Purpose: NETCON systems init
//-----------------------------------------------------------------------------
void CRConServer::Init(void)
{
	if (std::strlen(rcon_password->GetString()) < 8)
	{
		if (std::strlen(rcon_password->GetString()) > 0)
		{
			Warning(eDLL_T::SERVER, "Remote server access requires a password of at least 8 characters\n");
		}
		this->Shutdown();
		return;
	}

	static ConVar* hostport = g_pCVar->FindVar("hostport");

	m_pAdr2 = new CNetAdr2(rcon_address->GetString(), hostport->GetString());
	m_pSocket->CreateListenSocket(*m_pAdr2, false);
	m_svPasswordHash = sha256(rcon_password->GetString());

	DevMsg(eDLL_T::SERVER, "Remote server access initialized\n");
	m_bInitialized = true;
}

//-----------------------------------------------------------------------------
// Purpose: NETCON systems shutdown
//-----------------------------------------------------------------------------
void CRConServer::Shutdown(void)
{
	if (m_pSocket->IsListening())
	{
		m_pSocket->CloseListenSocket();
	}
	m_bInitialized = false;
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
					this->CloseConnection();
				}
			}
		}
	}

	// Create a new listen socket if authenticated connection is closed.
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
// Purpose: send message
// Input  : *svMessage - 
//-----------------------------------------------------------------------------
void CRConServer::Send(const std::string& svMessage) const
{
	int nCount = m_pSocket->GetAcceptedSocketCount();

	for (int i = nCount - 1; i >= 0; i--)
	{
		CConnectedNetConsoleData* pData = m_pSocket->GetAcceptedSocketData(i);

		if (pData->m_bAuthorized)
		{
			std::string svFinal = this->Serialize(svMessage, "", sv_rcon::response_t::SERVERDATA_RESPONSE_CONSOLE_LOG);
			::send(pData->m_hSocket, svFinal.c_str(), static_cast<int>(svFinal.size()), MSG_NOSIGNAL);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: receive message
//-----------------------------------------------------------------------------
void CRConServer::Recv(void)
{
	int nCount = m_pSocket->GetAcceptedSocketCount();
	static char szRecvBuf[MAX_NETCONSOLE_INPUT_LEN]{};

	for (m_nConnIndex = nCount - 1; m_nConnIndex >= 0; m_nConnIndex--)
	{
		CConnectedNetConsoleData* pData = m_pSocket->GetAcceptedSocketData(m_nConnIndex);
		{//////////////////////////////////////////////
			if (this->CheckForBan(pData))
			{
				std::string svNoAuth =  this->Serialize(s_pszBannedMessage, "", sv_rcon::response_t::SERVERDATA_RESPONSE_AUTH);
				::send(pData->m_hSocket, svNoAuth.c_str(), static_cast<int>(svNoAuth.size()), MSG_NOSIGNAL);
				this->CloseConnection();
				continue;
			}

			int nPendingLen = ::recv(pData->m_hSocket, szRecvBuf, sizeof(szRecvBuf), MSG_PEEK);
			if (nPendingLen == SOCKET_ERROR && m_pSocket->IsSocketBlocking())
			{
				continue;
			}
			if (nPendingLen <= 0) // EOF or error.
			{
				this->CloseConnection();
				continue;
			}
		}//////////////////////////////////////////////

		u_long nReadLen; // Find out how much we have to read.
		::ioctlsocket(pData->m_hSocket, FIONREAD, &nReadLen);

		while (nReadLen > 0)
		{
			memset(szRecvBuf, '\0', sizeof(szRecvBuf));
			int nRecvLen = ::recv(pData->m_hSocket, szRecvBuf, MIN(sizeof(szRecvBuf), nReadLen), MSG_NOSIGNAL);

			if (nRecvLen == 0) // Socket was closed.
			{
				this->CloseConnection();
				break;
			}
			if (nRecvLen < 0 && !m_pSocket->IsSocketBlocking())
			{
				break;
			}

			nReadLen -= nRecvLen; // Process what we've got.
			this->ProcessBuffer(szRecvBuf, nRecvLen, pData);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: serializes input
// Input  : *svRspBuf - 
//			*svRspVal - 
//			response_t - 
// Output : serialized results as string
//-----------------------------------------------------------------------------
std::string CRConServer::Serialize(const std::string& svRspBuf, const std::string& svRspVal, sv_rcon::response_t response_t) const
{
	sv_rcon::response sv_response;

	sv_response.set_responseid(-1); // TODO
	sv_response.set_responsetype(response_t);

	switch (response_t)
	{
		case sv_rcon::response_t::SERVERDATA_RESPONSE_AUTH:
		{
			sv_response.set_responsebuf(svRspBuf);
			break;
		}
		case sv_rcon::response_t::SERVERDATA_RESPONSE_CONSOLE_LOG:
		{
			sv_response.set_responsebuf(svRspBuf);
			sv_response.set_responseval("");
			break;
		}
		default:
		{
			break;
		}
	}
	return sv_response.SerializeAsString().append("\r");
}

//-----------------------------------------------------------------------------
// Purpose: de-serializes input
// Input  : *svBuf - 
// Output : de-serialized object
//-----------------------------------------------------------------------------
cl_rcon::request CRConServer::Deserialize(const std::string& svBuf) const
{
	cl_rcon::request cl_request;
	cl_request.ParseFromArray(svBuf.c_str(), static_cast<int>(svBuf.size()));

	return cl_request;
}

//-----------------------------------------------------------------------------
// Purpose: authenticate new connections
// Input  : *cl_request - 
//			*pData - 
// Todo   : implement logic for key exchange instead so we never network our
// password in plain text over the wire. create a cvar for this so user could
// also opt out and use legacy authentication instead for older RCON clients
//-----------------------------------------------------------------------------
void CRConServer::Authenticate(const cl_rcon::request& cl_request, CConnectedNetConsoleData* pData)
{
	if (pData->m_bAuthorized)
	{
		return;
	}
	else if (strcmp(cl_request.requestbuf().c_str(), "PASS") == 0)
	{
		if (this->Comparator(cl_request.requestval()))
		{
			pData->m_bAuthorized = true;
			m_pSocket->CloseListenSocket();
			this->CloseNonAuthConnection();

			std::string svAuth = this->Serialize(s_pszAuthMessage, "", sv_rcon::response_t::SERVERDATA_RESPONSE_AUTH);
			::send(pData->m_hSocket, svAuth.c_str(), static_cast<int>(svAuth.size()), MSG_NOSIGNAL);
		}
		else // Bad password.
		{
			CNetAdr2 netAdr2 = m_pSocket->GetAcceptedSocketAddress(m_nConnIndex);
			if (sv_rcon_debug->GetBool())
			{
				DevMsg(eDLL_T::SERVER, "Bad RCON password attempt from '%s'\n", netAdr2.GetIPAndPort().c_str());
			}

			std::string svWrongPass = this->Serialize(s_pszWrongPwMessage, "", sv_rcon::response_t::SERVERDATA_RESPONSE_AUTH);
			::send(pData->m_hSocket, svWrongPass.c_str(), static_cast<int>(svWrongPass.size()), MSG_NOSIGNAL);

			pData->m_bAuthorized = false;
			pData->m_nFailedAttempts++;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: sha256 hashed password comparison
// Input  : *svCompare - 
// Output : true if matches, false otherwise
//-----------------------------------------------------------------------------
bool CRConServer::Comparator(std::string svPassword) const
{
	svPassword = sha256(svPassword);
	if (sv_rcon_debug->GetBool())
	{
		DevMsg(eDLL_T::SERVER, "+---------------------------------------------------------------------------+\n");
		DevMsg(eDLL_T::SERVER, "] Server: '%s'[\n", m_svPasswordHash.c_str());
		DevMsg(eDLL_T::SERVER, "] Client: '%s'[\n", svPassword.c_str());
		DevMsg(eDLL_T::SERVER, "+---------------------------------------------------------------------------+\n");
	}
	if (memcmp(svPassword.c_str(), m_svPasswordHash.c_str(), SHA256::DIGEST_SIZE) == 0)
	{
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: handles input command buffer
// Input  : *pszIn - 
//			nRecvLen - 
//			*pData - 
//-----------------------------------------------------------------------------
void CRConServer::ProcessBuffer(const char* pszIn, int nRecvLen, CConnectedNetConsoleData* pData)
{
	while (nRecvLen)
	{
		switch (*pszIn)
		{
		case '\r':
		{
			if (pData->m_nCharsInCommandBuffer)
			{
				cl_rcon::request cl_request = this->Deserialize(pData->m_pszInputCommandBuffer);
				this->ProcessMessage(cl_request);
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
// Purpose: processes received message
// Input  : *cl_request - 
//-----------------------------------------------------------------------------
void CRConServer::ProcessMessage(const cl_rcon::request& cl_request)
{
	CConnectedNetConsoleData* pData = m_pSocket->GetAcceptedSocketData(m_nConnIndex);

	if (!pData->m_bAuthorized 
		&& cl_request.requesttype() != cl_rcon::request_t::SERVERDATA_REQUEST_AUTH)
	{
		// Notify net console that authentication is required.
		std::string svMessage = this->Serialize(s_pszNoAuthMessage, "", sv_rcon::response_t::SERVERDATA_RESPONSE_AUTH);
		::send(pData->m_hSocket, svMessage.c_str(), static_cast<int>(svMessage.size()), MSG_NOSIGNAL);

		pData->m_nIgnoredMessage++;
		return;
	}
	switch (cl_request.requesttype())
	{
		case cl_rcon::request_t::SERVERDATA_REQUEST_AUTH:
		{
			this->Authenticate(cl_request, pData);
			break;
		}
		case cl_rcon::request_t::SERVERDATA_REQUEST_EXECCOMMAND:
		case cl_rcon::request_t::SERVERDATA_REQUEST_SETVALUE:
		{
			// Only execute if auth was succesfull.
			if (pData->m_bAuthorized)
			{
				this->Execute(cl_request);
			}
			break;
		}
		case cl_rcon::request_t::SERVERDATA_REQUEST_SEND_CONSOLE_LOG:
		{
			if (pData->m_bAuthorized)
			{
				// TODO: Send conlog to true.
			}
			break;
		}
		default:
		{
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: execute commands issued from net console
// Input  : *cl_request - 
//-----------------------------------------------------------------------------
void CRConServer::Execute(const cl_rcon::request& cl_request) const
{
	ConVar* pConVar = g_pCVar->FindVar(cl_request.requestbuf().c_str());
	if (pConVar)
	{
		pConVar->SetValue(cl_request.requestval().c_str());
	}
	else // Execute command with "<val>".
	{
		std::string svExec = cl_request.requestbuf() + " \"" + cl_request.requestval() + "\"";

		Cbuf_AddText(Cbuf_GetCurrentPlayer(), svExec.c_str(), cmd_source_t::kCommandSrcCode);
		Cbuf_Execute();
	}
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
	CConnectedNetConsoleData* pData = m_pSocket->GetAcceptedSocketData(m_nConnIndex);
	if (pData->m_bAuthorized)
	{
		// Inform server owner when authenticated connection has been closed.
		CNetAdr2 netAdr2 = m_pSocket->GetAcceptedSocketAddress(m_nConnIndex);
		DevMsg(eDLL_T::SERVER, "Net console '%s' closed RCON connection\n", netAdr2.GetIPAndPort().c_str());
	}
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
