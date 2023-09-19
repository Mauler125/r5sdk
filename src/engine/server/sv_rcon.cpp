//===========================================================================//
// 
// Purpose: Implementation of the rcon server.
// 
//===========================================================================//

#include "core/stdafx.h"
#include "tier1/cvar.h"
#include "tier1/NetAdr.h"
#include "tier2/socketcreator.h"
#include "engine/cmd.h"
#include "engine/net.h"
#include "engine/server/sv_rcon.h"
#include "protoc/sv_rcon.pb.h"
#include "protoc/cl_rcon.pb.h"
#include "mathlib/sha256.h"
#include "common/igameserverdata.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static const char s_NoAuthMessage[]  = "This server is password protected for console access; authenticate with 'PASS <password>' command.\n";
static const char s_WrongPwMessage[] = "Admin password incorrect.\n";
static const char s_AuthMessage[]    = "Authentication successful.\n";
static const char s_BannedMessage[]  = "Go away.\n";

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CRConServer::CRConServer(void)
	: m_nConnIndex(0)
	, m_nAuthConnections(0)
	, m_bInitialized(false)
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
		if (!SetPassword(rcon_password->GetString()))
		{
			return;
		}
	}
	else
	{
		// Already initialized.
		return;
	}

	const char* pszAddress = net_usesocketsforloopback->GetBool() ? NET_IPV6_UNSPEC : NET_IPV6_LOOPBACK;

	m_Address.SetFromString(Format("[%s]:%i", pszAddress, hostport->GetInt()).c_str(), true);
	m_Socket.CreateListenSocket(m_Address);

	Msg(eDLL_T::SERVER, "Remote server access initialized ('%s')\n", m_Address.ToString());
	m_bInitialized = true;
}

//-----------------------------------------------------------------------------
// Purpose: NETCON systems shutdown
//-----------------------------------------------------------------------------
void CRConServer::Shutdown(void)
{
	m_Socket.CloseAllAcceptedSockets();
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
	if (nCount > sv_rcon_maxsockets->GetInt())
	{
		for (m_nConnIndex = nCount - 1; m_nConnIndex >= 0; m_nConnIndex--)
		{
			const netadr_t& netAdr = m_Socket.GetAcceptedSocketAddress(m_nConnIndex);
			if (!m_WhiteListAddress.CompareAdr(netAdr))
			{
				const CConnectedNetConsoleData& data = m_Socket.GetAcceptedSocketData(m_nConnIndex);
				if (!data.m_bAuthorized)
				{
					Disconnect("redundant");
				}
			}
		}
	}

	// Create a new listen socket if authenticated connection is closed.
	if (nCount == 0)
	{
		if (!m_Socket.IsListening())
		{
			m_Socket.CreateListenSocket(m_Address);
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

	const size_t nLen = strlen(pszPassword);
	if (nLen < RCON_MIN_PASSWORD_LEN)
	{
		if (nLen > NULL)
		{
			Warning(eDLL_T::SERVER, "Remote server access requires a password of at least %i characters\n",
				RCON_MIN_PASSWORD_LEN);
		}

		Shutdown();
		return false;
	}

	m_svPasswordHash = sha256(pszPassword);
	Msg(eDLL_T::SERVER, "Password hash ('%s')\n", m_svPasswordHash.c_str());

	m_bInitialized = true;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: sets the white list address
// Input  : *pszAddress - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CRConServer::SetWhiteListAddress(const char* pszAddress)
{
	return m_WhiteListAddress.SetFromString(pszAddress);
}

//-----------------------------------------------------------------------------
// Purpose: server RCON main loop (run this every frame)
//-----------------------------------------------------------------------------
void CRConServer::RunFrame(void)
{
	if (m_bInitialized)
	{
		m_Socket.RunFrame();
		Think();

		const int nCount = m_Socket.GetAcceptedSocketCount();
		for (m_nConnIndex = nCount - 1; m_nConnIndex >= 0; m_nConnIndex--)
		{
			CConnectedNetConsoleData& data = m_Socket.GetAcceptedSocketData(m_nConnIndex);

			if (CheckForBan(data))
			{
				SendEncode(data.m_hSocket, s_BannedMessage, "",
					sv_rcon::response_t::SERVERDATA_RESPONSE_AUTH, int(eDLL_T::NETCON));

				Disconnect("banned");
				continue;
			}

			Recv(data, sv_rcon_maxpacketsize->GetInt());
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: send message to all connected sockets
// Input  : *pMsgBuf - 
//			nMsgLen - 
// Output: true on success, false otherwise
//-----------------------------------------------------------------------------
bool CRConServer::SendToAll(const char* pMsgBuf, const int nMsgLen) const
{
	ostringstream sendbuf;
	const u_long nLen = htonl(u_long(nMsgLen));

	bool bSuccess = true;

	sendbuf.write(reinterpret_cast<const char*>(&nLen), sizeof(u_long));
	sendbuf.write(pMsgBuf, nMsgLen);

	const int nCount = m_Socket.GetAcceptedSocketCount();
	for (int i = nCount - 1; i >= 0; i--)
	{
		const CConnectedNetConsoleData& data = m_Socket.GetAcceptedSocketData(i);

		if (data.m_bAuthorized && !data.m_bInputOnly)
		{
			int ret = ::send(data.m_hSocket, sendbuf.str().data(),
				int(sendbuf.str().size()), MSG_NOSIGNAL);

			if (ret == SOCKET_ERROR)
			{
				if (!bSuccess)
				{
					bSuccess = false;
				}
			}
		}
	}

	return bSuccess;
}

//-----------------------------------------------------------------------------
// Purpose: encode and send message to all connected sockets
// Input  : *pResponseMsg - 
//			*pResponseVal - 
//			responseType - 
//			nMessageId - 
//			nMessageType - 
// Output: true on success, false otherwise
//-----------------------------------------------------------------------------
bool CRConServer::SendEncode(const char* pResponseMsg, const char* pResponseVal,
	const sv_rcon::response_t responseType, const int nMessageId, const int nMessageType) const
{
	vector<char> vecMsg;
	if (!Serialize(vecMsg, pResponseMsg, pResponseVal,
		responseType, nMessageId, nMessageType))
	{
		return false;
	}
	if (!SendToAll(vecMsg.data(), int(vecMsg.size())))
	{
		Error(eDLL_T::SERVER, NO_ERROR, "Failed to send RCON message: (%s)\n", "SOCKET_ERROR");
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: encode and send message to specific socket
// Input  : hSocket - 
//			*pResponseMsg - 
//			*pResponseVal - 
//			responseType - 
//			nMessageId - 
//			nMessageType - 
// Output: true on success, false otherwise
//-----------------------------------------------------------------------------
bool CRConServer::SendEncode(const SocketHandle_t hSocket, const char* pResponseMsg, const char* pResponseVal,
	const sv_rcon::response_t responseType, const int nMessageId, const int nMessageType) const
{
	vector<char> vecMsg;
	if (!Serialize(vecMsg, pResponseMsg, pResponseVal,
		responseType, nMessageId, nMessageType))
	{
		return false;
	}
	if (!Send(hSocket, vecMsg.data(), int(vecMsg.size())))
	{
		Error(eDLL_T::SERVER, NO_ERROR, "Failed to send RCON message: (%s)\n", "SOCKET_ERROR");
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: serializes input
// Input  : &vecBuf - 
//			*responseMsg - 
//			*responseVal - 
//			responseType - 
//			nMessageId - 
//			nMessageType - 
// Output : serialized results as string
//-----------------------------------------------------------------------------
bool CRConServer::Serialize(vector<char>& vecBuf, const char* pResponseMsg, const char* pResponseVal,
	const sv_rcon::response_t responseType, const int nMessageId, const int nMessageType) const
{
	sv_rcon::response response;

	response.set_messageid(nMessageId);
	response.set_messagetype(nMessageType);
	response.set_responsetype(responseType);
	response.set_responsemsg(pResponseMsg);
	response.set_responseval(pResponseVal);

	const size_t msgLen = response.ByteSizeLong();
	vecBuf.resize(msgLen);

	if (!Encode(&response, &vecBuf[0], msgLen))
	{
		Error(eDLL_T::SERVER, NO_ERROR, "Failed to encode RCON buffer\n");
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: authenticate new connections
// Input  : &request - 
//			&data - 
//-----------------------------------------------------------------------------
void CRConServer::Authenticate(const cl_rcon::request& request, CConnectedNetConsoleData& data)
{
	if (data.m_bAuthorized)
	{
		return;
	}
	else // Authorize.
	{
		if (Comparator(request.requestmsg()))
		{
			data.m_bAuthorized = true;
			if (++m_nAuthConnections >= sv_rcon_maxconnections->GetInt())
			{
				m_Socket.CloseListenSocket();
				CloseNonAuthConnection();
			}

			const char* pSendLogs = (!sv_rcon_sendlogs->GetBool() || data.m_bInputOnly) ? "0" : "1";

			SendEncode(data.m_hSocket, s_AuthMessage, pSendLogs,
				sv_rcon::response_t::SERVERDATA_RESPONSE_AUTH, static_cast<int>(eDLL_T::NETCON));
		}
		else // Bad password.
		{
			const netadr_t netAdr = m_Socket.GetAcceptedSocketAddress(m_nConnIndex);
			if (sv_rcon_debug->GetBool())
			{
				Msg(eDLL_T::SERVER, "Bad RCON password attempt from '%s'\n", netAdr.ToString());
			}

			SendEncode(data.m_hSocket, s_WrongPwMessage, "",
				sv_rcon::response_t::SERVERDATA_RESPONSE_AUTH, static_cast<int>(eDLL_T::NETCON));

			data.m_bAuthorized = false;
			data.m_bValidated = false;
			data.m_nFailedAttempts++;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: sha256 hashed password comparison
// Input  : &svPassword - 
// Output : true if matches, false otherwise
//-----------------------------------------------------------------------------
bool CRConServer::Comparator(const string& svPassword) const
{
	string passwordHash = sha256(svPassword);
	if (sv_rcon_debug->GetBool())
	{
		Msg(eDLL_T::SERVER, "+---------------------------------------------------------------------------+\n");
		Msg(eDLL_T::SERVER, "[ Server: '%s']\n", m_svPasswordHash.c_str());
		Msg(eDLL_T::SERVER, "[ Client: '%s']\n", passwordHash.c_str());
		Msg(eDLL_T::SERVER, "+---------------------------------------------------------------------------+\n");
	}
	if (memcmp(passwordHash.data(), m_svPasswordHash.data(), SHA256::DIGEST_SIZE) == 0)
	{
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: processes received message
// Input  : *pMsgBuf - 
//			nMsgLen - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CRConServer::ProcessMessage(const char* pMsgBuf, const int nMsgLen)
{
	CConnectedNetConsoleData& data = m_Socket.GetAcceptedSocketData(m_nConnIndex);

	cl_rcon::request request;
	if (!Decode(&request, pMsgBuf, nMsgLen))
	{
		Error(eDLL_T::SERVER, NO_ERROR, "Failed to decode RCON buffer\n");
		return false;
	}

	if (!data.m_bAuthorized &&
		request.requesttype() != cl_rcon::request_t::SERVERDATA_REQUEST_AUTH)
	{
		// Notify netconsole that authentication is required.
		SendEncode(data.m_hSocket, s_NoAuthMessage, "",
			sv_rcon::response_t::SERVERDATA_RESPONSE_AUTH, static_cast<int>(eDLL_T::NETCON));

		data.m_bValidated = false;
		data.m_nIgnoredMessage++;
		return true;
	}
	switch (request.requesttype())
	{
		case cl_rcon::request_t::SERVERDATA_REQUEST_AUTH:
		{
			Authenticate(request, data);
			break;
		}
		case cl_rcon::request_t::SERVERDATA_REQUEST_EXECCOMMAND:
		{
			if (data.m_bAuthorized) // Only execute if auth was successful.
			{
				Execute(request);
			}
			break;
		}
		case cl_rcon::request_t::SERVERDATA_REQUEST_SEND_CONSOLE_LOG:
		{
			if (data.m_bAuthorized)
			{
				// request value "0" means the netconsole is input only.
				const bool bWantLog = atoi(request.requestval().c_str()) != NULL;

				data.m_bInputOnly = !bWantLog;
				if (bWantLog && !sv_rcon_sendlogs->GetBool())
				{
					// Toggle it on since there's at least 1 netconsole that
					// wants to receive logs.
					sv_rcon_sendlogs->SetValue(bWantLog);
				}
			}
			break;
		}
		default:
		{
			break;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: execute commands issued from netconsole (ignores all protection flags)
// Input  : &request - 
//-----------------------------------------------------------------------------
void CRConServer::Execute(const cl_rcon::request& request) const
{
	const string& commandString = request.requestmsg().c_str();
	const char* const pCommandString = commandString.c_str();

	ConCommandBase* pCommandBase = g_pCVar->FindCommandBase(pCommandString);

	if (!pCommandBase)
	{
		// Found nothing.
		return;
	}

	const char* const pValueString = request.requestval().c_str();

	if (!pCommandBase->IsCommand())
	{
		// Here we want to skip over the command string in the value buffer.
		// So if we got 'sv_cheats 1' in our value buffer, we want to skip
		// over 'sv_cheats ', so that we are pointing directly to the value.
		const char* pFound = V_strstr(pValueString,  pCommandString);
		const char* pValue = nullptr;

		if (pFound)
		{
			pValue = pFound + commandString.length();

			// Skip any leading space characters.
			while (*pValue == ' ')
			{
				++pValue;
			}
		}

		ConVar* pConVar = reinterpret_cast<ConVar*>(pCommandBase);
		pConVar->SetValue(pValue ? pValue : pValueString);
	}
	else // Invoke command callback directly.
	{
		CCommand cmd;
		cmd.Tokenize(pValueString, cmd_source_t::kCommandSrcCode);

		v_Cmd_Dispatch(ECommandTarget_t::CBUF_SERVER, pCommandBase, &cmd, false);
	}
}

//-----------------------------------------------------------------------------
// Purpose: checks for amount of failed attempts and bans netconsole accordingly
// Input  : &data - 
//-----------------------------------------------------------------------------
bool CRConServer::CheckForBan(CConnectedNetConsoleData& data)
{
	if (data.m_bValidated)
	{
		return false;
	}

	const netadr_t netAdr = m_Socket.GetAcceptedSocketAddress(m_nConnIndex);
	const char* szNetAdr = netAdr.ToString(true);

	if (m_BannedList.size() >= RCON_MAX_BANNEDLIST_SIZE)
	{
		const char* pszWhiteListAddress = sv_rcon_whitelist_address->GetString();
		if (!pszWhiteListAddress[0])
		{
			Warning(eDLL_T::SERVER, "Banned list overflowed; please use a whitelist address. RCON shutting down...\n");
			Shutdown();

			return true;
		}

		// Only allow whitelisted at this point.
		if (!m_WhiteListAddress.CompareAdr(netAdr))
		{
			if (sv_rcon_debug->GetBool())
			{
				Msg(eDLL_T::SERVER, "Banned list is full; dropping '%s'\n", szNetAdr);
			}

			return true;
		}
	}

	data.m_bValidated = true;

	// Check if IP is in the banned list.
	if (m_BannedList.find(szNetAdr) != m_BannedList.end())
	{
		return true;
	}

	// Check if netconsole has reached maximum number of attempts > add to banned list.
	if (data.m_nFailedAttempts >= sv_rcon_maxfailures->GetInt()
		|| data.m_nIgnoredMessage >= sv_rcon_maxignores->GetInt())
	{
		// Don't add white listed address to banned list.
		if (m_WhiteListAddress.CompareAdr(netAdr))
		{
			data.m_nFailedAttempts = 0;
			data.m_nIgnoredMessage = 0;
			return false;
		}

		Warning(eDLL_T::SERVER, "Banned '%s' for RCON hacking attempts\n", szNetAdr);
		m_BannedList.insert(szNetAdr);

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: close connection on current index
//-----------------------------------------------------------------------------
void CRConServer::Disconnect(const char* szReason) // NETMGR
{
	Disconnect(m_nConnIndex, szReason);
}

//-----------------------------------------------------------------------------
// Purpose: close specific connection by index
//-----------------------------------------------------------------------------
void CRConServer::Disconnect(const int nIndex, const char* szReason) // NETMGR
{
	CConnectedNetConsoleData& data = m_Socket.GetAcceptedSocketData(nIndex);
	if (data.m_bAuthorized)
	{
		// Inform server owner when authenticated connection has been closed.
		netadr_t netAdr = m_Socket.GetAcceptedSocketAddress(nIndex);
		if (!szReason)
		{
			szReason = "unknown reason";
		}

		Msg(eDLL_T::SERVER, "Connection to '%s' lost (%s)\n", netAdr.ToString(), szReason);
		m_nAuthConnections--;
	}

	m_Socket.CloseAcceptedSocket(nIndex);
}

//-----------------------------------------------------------------------------
// Purpose: close all connections except for authenticated
//-----------------------------------------------------------------------------
void CRConServer::CloseNonAuthConnection(void)
{
	int nCount = m_Socket.GetAcceptedSocketCount();
	for (int i = nCount - 1; i >= 0; i--)
	{
		CConnectedNetConsoleData& data = m_Socket.GetAcceptedSocketData(i);

		if (!data.m_bAuthorized)
		{
			m_Socket.CloseAcceptedSocket(i);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: checks if this message should be send or not
// Input  : responseType -
// Output : true if it should send, false otherwise
//-----------------------------------------------------------------------------
bool CRConServer::ShouldSend(const sv_rcon::response_t responseType) const
{
	if (!IsInitialized() || !m_Socket.GetAcceptedSocketCount())
	{
		// Not initialized or no sockets...
		return false;
	}

	if (responseType == sv_rcon::response_t::SERVERDATA_RESPONSE_CONSOLE_LOG)
	{
		if (!sv_rcon_sendlogs->GetBool() || !m_Socket.GetAuthorizedSocketCount())
		{
			// Disabled or no authorized clients to send to...
			return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: returns whether the rcon server is initialized
//-----------------------------------------------------------------------------
bool CRConServer::IsInitialized(void) const
{
	return m_bInitialized;
}

//-----------------------------------------------------------------------------
// Purpose: returns the number of authenticated connections
//-----------------------------------------------------------------------------
int CRConServer::GetAuthenticatedCount(void) const
{
	return m_nAuthConnections;
}

///////////////////////////////////////////////////////////////////////////////
CRConServer* g_RCONServer(new CRConServer());
CRConServer* RCONServer() // Singleton RCON Server.
{
	return g_RCONServer;
}
