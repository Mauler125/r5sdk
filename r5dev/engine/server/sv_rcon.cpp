//===========================================================================//
// 
// Purpose: Implementation of the rcon server.
// 
//===========================================================================//

#include "core/stdafx.h"
#include "tier1/cmd.h"
#include "tier1/cvar.h"
#include "tier1/IConVar.h"
#include "tier1/NetAdr.h"
#include "tier2/socketcreator.h"
#include "engine/net.h"
#include "engine/server/sv_rcon.h"
#include "protoc/sv_rcon.pb.h"
#include "protoc/cl_rcon.pb.h"
#include "mathlib/sha256.h"
#include "common/igameserverdata.h"


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CRConServer::CRConServer(void)
	: m_bInitialized(false)
	, m_nConnIndex(0)
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CRConServer::~CRConServer(void)
{
}

//-----------------------------------------------------------------------------
// Purpose: NETCON systems init
//-----------------------------------------------------------------------------
void CRConServer::Init(void)
{
	if (!m_bInitialized)
	{
		if (!this->SetPassword(rcon_password->GetString()))
		{
			return;
		}
	}

	m_Address.SetFromString(hostip->GetString(), true);
	m_Address.SetPort(htons(hostport->GetInt()));

	m_Socket.CreateListenSocket(m_Address, false);

	DevMsg(eDLL_T::SERVER, "Remote server access initialized ('%s')\n", m_Address.ToString());
	m_bInitialized = true;
}

//-----------------------------------------------------------------------------
// Purpose: NETCON systems shutdown
//-----------------------------------------------------------------------------
void CRConServer::Shutdown(void)
{
	if (m_Socket.IsListening())
	{
		m_Socket.CloseListenSocket();
	}
	m_bInitialized = false;
}

//-----------------------------------------------------------------------------
// Purpose: run tasks for the RCON server
//-----------------------------------------------------------------------------
void CRConServer::Think(void)
{
	const int nCount = m_Socket.GetAcceptedSocketCount();

	// Close redundant sockets if there are too many except for whitelisted and authenticated.
	if (nCount >= sv_rcon_maxsockets->GetInt())
	{
		for (m_nConnIndex = nCount - 1; m_nConnIndex >= 0; m_nConnIndex--)
		{
			const netadr_t& netAdr = m_Socket.GetAcceptedSocketAddress(m_nConnIndex);
			if (strcmp(netAdr.ToString(true), sv_rcon_whitelist_address->GetString()) != 0) // TODO: doesn't work
			{
				const CConnectedNetConsoleData* pData = m_Socket.GetAcceptedSocketData(m_nConnIndex);
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
		if (!m_Socket.IsListening())
		{
			m_Socket.CreateListenSocket(m_Address, false);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: changes the password
// Input  : *pszPassword - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CRConServer::SetPassword(const char* pszPassword)
{
	m_bInitialized = false;
	m_Socket.CloseAllAcceptedSockets();

	const size_t nLen = std::strlen(pszPassword);
	if (nLen < 8)
	{
		if (nLen > 0)
		{
			Warning(eDLL_T::SERVER, "Remote server access requires a password of at least 8 characters\n");
		}

		this->Shutdown();
		return false;
	}

	m_svPasswordHash = sha256(pszPassword);
	DevMsg(eDLL_T::SERVER, "Password hash ('%s')\n", m_svPasswordHash.c_str());

	m_bInitialized = true;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: server RCON main loop (run this every frame)
//-----------------------------------------------------------------------------
void CRConServer::RunFrame(void)
{
	if (m_bInitialized)
	{
		m_Socket.RunFrame();
		this->Think();
		this->Recv();
	}
}

//-----------------------------------------------------------------------------
// Purpose: send message to all connected sockets
// Input  : *svMessage - 
//-----------------------------------------------------------------------------
void CRConServer::Send(const std::string& svMessage) const
{
	if (const int nCount = m_Socket.GetAcceptedSocketCount())
	{
		std::ostringstream ssSendBuf;
		const u_long nLen = htonl(svMessage.size());

		ssSendBuf.write(reinterpret_cast<const char*>(&nLen), sizeof(u_long));
		ssSendBuf.write(svMessage.data(), svMessage.size());

		for (int i = nCount - 1; i >= 0; i--)
		{
			CConnectedNetConsoleData* pData = m_Socket.GetAcceptedSocketData(i);

			if (pData->m_bAuthorized)
			{
				::send(pData->m_hSocket, ssSendBuf.str().data(), static_cast<int>(ssSendBuf.str().size()), MSG_NOSIGNAL);
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: send message to specific connected socket
// Input  : hSocket - 
//			*svMessage - 
//-----------------------------------------------------------------------------
void CRConServer::Send(const SocketHandle_t hSocket, const std::string& svMessage) const
{
	std::ostringstream ssSendBuf;
	const u_long nLen = htonl(svMessage.size());

	ssSendBuf.write(reinterpret_cast<const char*>(&nLen), sizeof(u_long));
	ssSendBuf.write(svMessage.data(), svMessage.size());

	::send(hSocket, ssSendBuf.str().data(), static_cast<int>(ssSendBuf.str().size()), MSG_NOSIGNAL);
}

//-----------------------------------------------------------------------------
// Purpose: send serialized message to all connected sockets
// Input  : *svRspBuf - 
//			*svRspVal - 
//			responseType - 
//			nResponseId - 
//-----------------------------------------------------------------------------
void CRConServer::Send(const std::string& svRspBuf, const std::string& svRspVal, const sv_rcon::response_t responseType, const int nResponseId)
{
	if (responseType == sv_rcon::response_t::SERVERDATA_RESPONSE_CONSOLE_LOG)
	{
		if (!sv_rcon_sendlogs->GetBool())
		{
			return;
		}
	}
	this->Send(this->Serialize(svRspBuf, svRspVal, responseType, nResponseId));
}

//-----------------------------------------------------------------------------
// Purpose: send serialized message to specific connected socket
// Input  : hSocket - 
//			*svRspBuf - 
//			*svRspVal - 
//			responseType - 
//			nResponseId - 
//-----------------------------------------------------------------------------
void CRConServer::Send(const SocketHandle_t hSocket, const std::string& svRspBuf, const std::string& svRspVal, const sv_rcon::response_t responseType, const int nResponseId)
{
	if (responseType == sv_rcon::response_t::SERVERDATA_RESPONSE_CONSOLE_LOG)
	{
		if (!sv_rcon_sendlogs->GetBool())
		{
			return;
		}
	}
	this->Send(hSocket, this->Serialize(svRspBuf, svRspVal, responseType, nResponseId));
}

//-----------------------------------------------------------------------------
// Purpose: receive message
//-----------------------------------------------------------------------------
void CRConServer::Recv(void)
{
	const int nCount = m_Socket.GetAcceptedSocketCount();
	static char szRecvBuf[1024];

	for (m_nConnIndex = nCount - 1; m_nConnIndex >= 0; m_nConnIndex--)
	{
		CConnectedNetConsoleData* pData = m_Socket.GetAcceptedSocketData(m_nConnIndex);
		{//////////////////////////////////////////////
			if (this->CheckForBan(pData))
			{
				this->Send(pData->m_hSocket, this->Serialize(s_pszBannedMessage, "", sv_rcon::response_t::SERVERDATA_RESPONSE_AUTH, static_cast<int>(EGlobalContext_t::NETCON_S)));
				this->CloseConnection();
				continue;
			}

			const int nPendingLen = ::recv(pData->m_hSocket, szRecvBuf, sizeof(char), MSG_PEEK);
			if (nPendingLen == SOCKET_ERROR && m_Socket.IsSocketBlocking())
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
			const int nRecvLen = ::recv(pData->m_hSocket, szRecvBuf, MIN(sizeof(szRecvBuf), nReadLen), MSG_NOSIGNAL);
			if (nRecvLen == 0) // Socket was closed.
			{
				this->CloseConnection();
				break;
			}
			if (nRecvLen < 0 && !m_Socket.IsSocketBlocking())
			{
				Error(eDLL_T::SERVER, NO_ERROR, "RCON Cmd: recv error (%s)\n", NET_ErrorString(WSAGetLastError()));
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
//			responseType - 
// Output : serialized results as string
//-----------------------------------------------------------------------------
std::string CRConServer::Serialize(const std::string& svRspBuf, const std::string& svRspVal, const sv_rcon::response_t responseType, const int nResponseId) const
{
	sv_rcon::response sv_response;

	sv_response.set_responseid(nResponseId);
	sv_response.set_responsetype(responseType);

	switch (responseType)
	{
		case sv_rcon::response_t::SERVERDATA_RESPONSE_AUTH:
		{
			sv_response.set_responsebuf(svRspBuf);
			sv_response.set_responseval(svRspVal);
			break;
		}
		case sv_rcon::response_t::SERVERDATA_RESPONSE_CONSOLE_LOG:
		{
			sv_response.set_responsebuf(svRspBuf);
			sv_response.set_responseval(svRspVal);
			break;
		}
		default:
		{
			break;
		}
	}
	return sv_response.SerializeAsString();
}

//-----------------------------------------------------------------------------
// Purpose: de-serializes input
// Input  : *svBuf - 
// Output : de-serialized object
//-----------------------------------------------------------------------------
cl_rcon::request CRConServer::Deserialize(const std::string& svBuf) const
{
	cl_rcon::request cl_request;
	cl_request.ParseFromArray(svBuf.data(), static_cast<int>(svBuf.size()));

	return cl_request;
}

//-----------------------------------------------------------------------------
// Purpose: authenticate new connections
// Input  : *cl_request - 
//			*pData - 
//-----------------------------------------------------------------------------
void CRConServer::Authenticate(const cl_rcon::request& cl_request, CConnectedNetConsoleData* pData)
{
	if (pData->m_bAuthorized)
	{
		return;
	}
	else // Authorize.
	{
		if (this->Comparator(cl_request.requestbuf()))
		{
			pData->m_bAuthorized = true;
			m_Socket.CloseListenSocket();

			this->CloseNonAuthConnection();
			this->Send(pData->m_hSocket, this->Serialize(s_pszAuthMessage, sv_rcon_sendlogs->GetString(), sv_rcon::response_t::SERVERDATA_RESPONSE_AUTH, static_cast<int>(EGlobalContext_t::NETCON_S)));
		}
		else // Bad password.
		{
			const netadr_t netAdr = m_Socket.GetAcceptedSocketAddress(m_nConnIndex);
			if (sv_rcon_debug->GetBool())
			{
				DevMsg(eDLL_T::SERVER, "Bad RCON password attempt from '%s'\n", netAdr.ToString());
			}

			this->Send(pData->m_hSocket, this->Serialize(s_pszWrongPwMessage, "", sv_rcon::response_t::SERVERDATA_RESPONSE_AUTH, static_cast<int>(EGlobalContext_t::NETCON_S)));

			pData->m_bAuthorized = false;
			pData->m_bValidated = false;
			pData->m_nFailedAttempts++;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: sha256 hashed password comparison
// Input  : svCompare - 
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
	if (std::memcmp(svPassword.data(), m_svPasswordHash.data(), SHA256::DIGEST_SIZE) == 0)
	{
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: parses input response buffer using length-prefix framing
// Input  : *pRecvBuf - 
//			nRecvLen - 
//			*pData - 
//-----------------------------------------------------------------------------
void CRConServer::ProcessBuffer(const char* pRecvBuf, int nRecvLen, CConnectedNetConsoleData* pData)
{
	while (nRecvLen > 0)
	{
		if (pData->m_nPayloadLen)
		{
			if (pData->m_nPayloadRead < pData->m_nPayloadLen)
			{
				pData->m_RecvBuffer[pData->m_nPayloadRead++] = *pRecvBuf;

				pRecvBuf++;
				nRecvLen--;
			}
			if (pData->m_nPayloadRead == pData->m_nPayloadLen)
			{
				this->ProcessMessage(this->Deserialize(std::string(
					reinterpret_cast<char*>(pData->m_RecvBuffer.data()), pData->m_nPayloadLen)));

				pData->m_nPayloadLen = 0;
				pData->m_nPayloadRead = 0;
			}
		}
		else if (pData->m_nPayloadRead < sizeof(u_long)) // Read size field.
		{
			pData->m_RecvBuffer[pData->m_nPayloadRead++] = *pRecvBuf;

			pRecvBuf++;
			nRecvLen--;
		}
		else // Build prefix.
		{
			pData->m_nPayloadLen = ntohl(*reinterpret_cast<u_long*>(&pData->m_RecvBuffer[0]));
			pData->m_nPayloadRead = 0;

			if (!pData->m_bAuthorized)
			{
				if (pData->m_nPayloadLen > MAX_NETCONSOLE_INPUT_LEN)
				{
					this->CloseConnection(); // Sending large messages while not authenticated.
					break;
				}
			}

			if (pData->m_nPayloadLen < 0 ||
				pData->m_nPayloadLen > pData->m_RecvBuffer.max_size())
			{
				Error(eDLL_T::SERVER, NO_ERROR, "RCON Cmd: sync error (%d)\n", pData->m_nPayloadLen);
				this->CloseConnection(); // Out of sync (irrecoverable).

				break;
			}
			else
			{
				pData->m_RecvBuffer.resize(pData->m_nPayloadLen);
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: processes received message
// Input  : *cl_request - 
//-----------------------------------------------------------------------------
void CRConServer::ProcessMessage(const cl_rcon::request& cl_request)
{
	CConnectedNetConsoleData* pData = m_Socket.GetAcceptedSocketData(m_nConnIndex);

	if (!pData->m_bAuthorized 
		&& cl_request.requesttype() != cl_rcon::request_t::SERVERDATA_REQUEST_AUTH)
	{
		// Notify net console that authentication is required.
		this->Send(pData->m_hSocket, this->Serialize(s_pszNoAuthMessage, "", sv_rcon::response_t::SERVERDATA_RESPONSE_AUTH, static_cast<int>(EGlobalContext_t::NETCON_S)));

		pData->m_bValidated = false;
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
		{
			if (pData->m_bAuthorized) // Only execute if auth was successful.
			{
				this->Execute(cl_request, false);
			}
			break;
		}
		case cl_rcon::request_t::SERVERDATA_REQUEST_SETVALUE:
		{
			if (pData->m_bAuthorized)
			{
				this->Execute(cl_request, true);
			}
			break;
		}
		case cl_rcon::request_t::SERVERDATA_REQUEST_SEND_CONSOLE_LOG:
		{
			if (pData->m_bAuthorized)
			{
				sv_rcon_sendlogs->SetValue(true);
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
//			bConVar - 
//-----------------------------------------------------------------------------
void CRConServer::Execute(const cl_rcon::request& cl_request, const bool bConVar) const
{
	if (bConVar)
	{
		ConVar* pConVar = g_pCVar->FindVar(cl_request.requestbuf().c_str());
		if (pConVar) // Only run if this is a ConVar.
		{
			pConVar->SetValue(cl_request.requestval().c_str());
		}
	}
	else // Execute command with "<val>".
	{
		Cbuf_AddText(Cbuf_GetCurrentPlayer(), cl_request.requestbuf().c_str(), cmd_source_t::kCommandSrcCode);
	}
}

//-----------------------------------------------------------------------------
// Purpose: checks for amount of failed attempts and bans net console accordingly
// Input  : *pData - 
//-----------------------------------------------------------------------------
bool CRConServer::CheckForBan(CConnectedNetConsoleData* pData)
{
	if (pData->m_bValidated)
	{
		return false;
	}

	pData->m_bValidated = true;
	netadr_t netAdr = m_Socket.GetAcceptedSocketAddress(m_nConnIndex);

	// Check if IP is in the banned list.
	if (std::find(m_BannedList.begin(), m_BannedList.end(),
		netAdr.ToString(true)) != m_BannedList.end())
	{
		return true;
	}

	// Check if net console has reached maximum number of attempts > add to banned list.
	if (pData->m_nFailedAttempts >= sv_rcon_maxfailures->GetInt()
		|| pData->m_nIgnoredMessage >= sv_rcon_maxignores->GetInt())
	{
		// Don't add white listed address to banned list.
		if (strcmp(netAdr.ToString(true), sv_rcon_whitelist_address->GetString()) == 0)
		{
			pData->m_nFailedAttempts = 0;
			pData->m_nIgnoredMessage = 0;
			return false;
		}

		DevMsg(eDLL_T::SERVER, "Banned '%s' for RCON hacking attempts\n", netAdr.ToString());
		m_BannedList.push_back(netAdr.ToString(true));
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: close specific connection
//-----------------------------------------------------------------------------
void CRConServer::CloseConnection(void) // NETMGR
{
	CConnectedNetConsoleData* pData = m_Socket.GetAcceptedSocketData(m_nConnIndex);
	if (pData->m_bAuthorized)
	{
		// Inform server owner when authenticated connection has been closed.
		netadr_t netAdr = m_Socket.GetAcceptedSocketAddress(m_nConnIndex);
		DevMsg(eDLL_T::SERVER, "Net console '%s' closed RCON connection\n", netAdr.ToString());
	}
	m_Socket.CloseAcceptedSocket(m_nConnIndex);
}

//-----------------------------------------------------------------------------
// Purpose: close all connections except for authenticated
//-----------------------------------------------------------------------------
void CRConServer::CloseNonAuthConnection(void)
{
	int nCount = m_Socket.GetAcceptedSocketCount();
	for (int i = nCount - 1; i >= 0; i--)
	{
		CConnectedNetConsoleData* pData = m_Socket.GetAcceptedSocketData(i);

		if (!pData->m_bAuthorized)
		{
			m_Socket.CloseAcceptedSocket(i);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: checks if server rcon is initialized
// Output : true if initialized, false otherwise
//-----------------------------------------------------------------------------
bool CRConServer::IsInitialized(void) const
{
	return m_bInitialized;
}

///////////////////////////////////////////////////////////////////////////////
CRConServer g_RCONServer;
CRConServer* RCONServer() // Singleton RCON Server.
{
	return &g_RCONServer;
}
