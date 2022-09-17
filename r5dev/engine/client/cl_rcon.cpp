//===========================================================================//
// 
// Purpose: Implementation of the rcon client.
// 
//===========================================================================//

#include "core/stdafx.h"
#include "tier1/IConVar.h"
#include "tier1/cmd.h"
#include "tier1/cvar.h"
#include "protoc/sv_rcon.pb.h"
#include "protoc/cl_rcon.pb.h"
#include "engine/client/cl_rcon.h"
#include "engine/net.h"
#include "squirrel/sqvm.h"
#include "common/igameserverdata.h"


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CRConClient::CRConClient()
	: m_bInitialized(false)
	, m_bConnEstablished(false)
{
	m_pNetAdr2 = new CNetAdr2("localhost", "37015");
	m_pSocket = new CSocketCreator();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CRConClient::~CRConClient(void)
{
	delete m_pNetAdr2;
	delete m_pSocket;
}

//-----------------------------------------------------------------------------
// Purpose: NETCON systems init
//-----------------------------------------------------------------------------
void CRConClient::Init(void)
{
	if (!m_bInitialized)
	{
		this->SetPassword(rcon_password->GetString());
	}
	m_bInitialized = true;
}

//-----------------------------------------------------------------------------
// Purpose: NETCON systems shutdown
//-----------------------------------------------------------------------------
void CRConClient::Shutdown(void)
{
	if (m_bConnEstablished)
	{
		this->Disconnect();
	}
}

//-----------------------------------------------------------------------------
// Purpose: changes the password
// Input  : *pszPassword - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CRConClient::SetPassword(const char* pszPassword)
{
	size_t nLen = std::strlen(pszPassword);
	if (nLen < 8)
	{
		if (nLen > 0)
		{
			DevMsg(eDLL_T::CLIENT, "Remote server access requires a password of at least 8 characters\n");
		}
		return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: client rcon main processing loop
//-----------------------------------------------------------------------------
void CRConClient::RunFrame(void)
{
	if (m_bInitialized && m_bConnEstablished)
	{
		this->Recv();
	}
}

//-----------------------------------------------------------------------------
// Purpose: connect to address and port stored in 'rcon_address' cvar
// Output : true if connection succeeds, false otherwise
//-----------------------------------------------------------------------------
bool CRConClient::Connect(void)
{
	if (strlen(rcon_address->GetString()) > 0)
	{
		// Default is [127.0.0.1]:37015
		m_pNetAdr2->SetIPAndPort(rcon_address->GetString());
	}

	if (m_pSocket->ConnectSocket(*m_pNetAdr2, true) == SOCKET_ERROR)
	{
		Warning(eDLL_T::CLIENT, "Connection to RCON server failed: (SOCKET_ERROR)\n");
		return false;
	}
	DevMsg(eDLL_T::CLIENT, "Connected to: %s\n", m_pNetAdr2->GetIPAndPort().c_str());

	m_bConnEstablished = true;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: connect to specified address and port
// Input  : *svInAdr - 
//			*svInPort - 
// Output : true if connection succeeds, false otherwise
//-----------------------------------------------------------------------------
bool CRConClient::Connect(const std::string& svInAdr, const std::string& svInPort)
{
	if (!svInAdr.empty() && !svInPort.empty())
	{
		// Default is [127.0.0.1]:37015
		m_pNetAdr2->SetIPAndPort(svInAdr, svInPort);
	}

	if (m_pSocket->ConnectSocket(*m_pNetAdr2, true) == SOCKET_ERROR)
	{
		Warning(eDLL_T::CLIENT, "Connection to RCON server failed: (SOCKET_ERROR)\n");
		return false;
	}
	DevMsg(eDLL_T::CLIENT, "Connected to: %s\n", m_pNetAdr2->GetIPAndPort().c_str());

	m_bConnEstablished = true;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: disconnect from current session
//-----------------------------------------------------------------------------
void CRConClient::Disconnect(void)
{
	m_pSocket->CloseAcceptedSocket(0);
	m_bConnEstablished = false;
}

//-----------------------------------------------------------------------------
// Purpose: send message
// Input  : *svMessage - 
//-----------------------------------------------------------------------------
void CRConClient::Send(const std::string& svMessage) const
{
	std::ostringstream ssSendBuf;

	ssSendBuf << static_cast<uint8_t>(static_cast<int>(svMessage.size()) >> 24);
	ssSendBuf << static_cast<uint8_t>(static_cast<int>(svMessage.size()) >> 16);
	ssSendBuf << static_cast<uint8_t>(static_cast<int>(svMessage.size()) >> 8 );
	ssSendBuf << static_cast<uint8_t>(static_cast<int>(svMessage.size()));
	ssSendBuf << svMessage;

	int nSendResult = ::send(m_pSocket->GetAcceptedSocketData(0)->m_hSocket,
		ssSendBuf.str().data(), static_cast<int>(ssSendBuf.str().size()), MSG_NOSIGNAL);
	if (nSendResult == SOCKET_ERROR)
	{
		Warning(eDLL_T::CLIENT, "Failed to send RCON message: (SOCKET_ERROR)\n");
	}
}

//-----------------------------------------------------------------------------
// Purpose: receive message
//-----------------------------------------------------------------------------
void CRConClient::Recv(void)
{
	static char szRecvBuf[1024];
	CConnectedNetConsoleData* pData = m_pSocket->GetAcceptedSocketData(0);

	{//////////////////////////////////////////////
		int nPendingLen = ::recv(pData->m_hSocket, szRecvBuf, sizeof(char), MSG_PEEK);
		if (nPendingLen == SOCKET_ERROR && m_pSocket->IsSocketBlocking())
		{
			return;
		}
		if (nPendingLen <= 0 && m_bConnEstablished) // EOF or error.
		{
			this->Disconnect();
			DevMsg(eDLL_T::CLIENT, "Server closed RCON connection\n");
			return;
		}
	}//////////////////////////////////////////////

	u_long nReadLen; // Find out how much we have to read.
	::ioctlsocket(pData->m_hSocket, FIONREAD, &nReadLen);

	while (nReadLen > 0)
	{
		int nRecvLen = ::recv(pData->m_hSocket, szRecvBuf, MIN(sizeof(szRecvBuf), nReadLen), MSG_NOSIGNAL);
		if (nRecvLen == 0 && m_bConnEstablished) // Socket was closed.
		{
			this->Disconnect();
			DevMsg(eDLL_T::CLIENT, "Server closed RCON connection\n");
			break;
		}
		if (nRecvLen < 0 && !m_pSocket->IsSocketBlocking())
		{
			Error(eDLL_T::CLIENT, NO_ERROR, "RCON Cmd: recv error (%s)\n", NET_ErrorString(WSAGetLastError()));
			break;
		}

		nReadLen -= nRecvLen; // Process what we've got.
		this->ProcessBuffer(szRecvBuf, nRecvLen, pData);
	}
}

//-----------------------------------------------------------------------------
// Purpose: parses input response buffer using length-prefix framing
// Input  : *pszIn - 
//			nRecvLen - 
//			*pData - 
//-----------------------------------------------------------------------------
void CRConClient::ProcessBuffer(const char* pRecvBuf, int nRecvLen, CConnectedNetConsoleData* pData)
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
		else if (pData->m_nPayloadRead < sizeof(int)) // Read size field.
		{
			pData->m_RecvBuffer[pData->m_nPayloadRead++] = *pRecvBuf;

			pRecvBuf++;
			nRecvLen--;
		}
		else // Build prefix.
		{
			pData->m_nPayloadLen = static_cast<int>(
				pData->m_RecvBuffer[0] << 24 |
				pData->m_RecvBuffer[1] << 16 |
				pData->m_RecvBuffer[2] << 8  |
				pData->m_RecvBuffer[3]);
			pData->m_nPayloadRead = 0;

			if (pData->m_nPayloadLen < 0 ||
				pData->m_nPayloadLen > pData->m_RecvBuffer.max_size())
			{
				Error(eDLL_T::CLIENT, NO_ERROR, "RCON Cmd: sync error (%d)\n", pData->m_nPayloadLen);
				this->Disconnect(); // Out of sync (irrecoverable).

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
// Input  : *sv_response - 
//-----------------------------------------------------------------------------
void CRConClient::ProcessMessage(const sv_rcon::response& sv_response) const
{
	std::string svOut = sv_response.responsebuf();

	switch (sv_response.responsetype())
	{
	case sv_rcon::response_t::SERVERDATA_RESPONSE_AUTH:
	{
		if (!sv_response.responseval().empty())
		{
			long i = strtol(sv_response.responseval().c_str(), NULL, NULL);
			if (!i) // sv_rcon_sendlogs is not set.
			{
				if (cl_rcon_request_sendlogs->GetBool())
				{
					string svLogQuery = this->Serialize("", "", cl_rcon::request_t::SERVERDATA_REQUEST_SEND_CONSOLE_LOG);
					this->Send(svLogQuery);
				}
			}
		}

		DevMsg(eDLL_T::NETCON, "%s", svOut.c_str());
		break;
	}
	case sv_rcon::response_t::SERVERDATA_RESPONSE_CONSOLE_LOG:
	{
		NetMsg(static_cast<EGlobalContext_t>(sv_response.responseid()), svOut.c_str());
		break;
	}
	default:
	{
		break;
	}
	}
}

//-----------------------------------------------------------------------------
// Purpose: serializes input
// Input  : *svReqBuf - 
//			*svReqVal - 
//			request_t - 
// Output : serialized results as string
//-----------------------------------------------------------------------------
std::string CRConClient::Serialize(const std::string& svReqBuf, const std::string& svReqVal, cl_rcon::request_t request_t) const
{
	cl_rcon::request cl_request;

	cl_request.set_requestid(-1);
	cl_request.set_requesttype(request_t);

	switch (request_t)
	{
	case cl_rcon::request_t::SERVERDATA_REQUEST_SETVALUE:
	case cl_rcon::request_t::SERVERDATA_REQUEST_AUTH:
	{
		cl_request.set_requestbuf(svReqBuf);
		cl_request.set_requestval(svReqVal);
		break;
	}
	case cl_rcon::request_t::SERVERDATA_REQUEST_EXECCOMMAND:
	{
		cl_request.set_requestbuf(svReqBuf);
		break;
	}
	}
	return cl_request.SerializeAsString();
}

//-----------------------------------------------------------------------------
// Purpose: de-serializes input
// Input  : *svBuf - 
// Output : de-serialized object
//-----------------------------------------------------------------------------
sv_rcon::response CRConClient::Deserialize(const std::string& svBuf) const
{
	sv_rcon::response sv_response;
	sv_response.ParseFromArray(svBuf.data(), static_cast<int>(svBuf.size()));

	return sv_response;
}

//-----------------------------------------------------------------------------
// Purpose: checks if client rcon is initialized
// Output : true if initialized, false otherwise
//-----------------------------------------------------------------------------
bool CRConClient::IsInitialized(void) const
{
	return m_bInitialized;
}

//-----------------------------------------------------------------------------
// Purpose: checks if client rcon is connected
// Output : true if connected, false otherwise
//-----------------------------------------------------------------------------
bool CRConClient::IsConnected(void) const
{
	return m_bConnEstablished;
}
///////////////////////////////////////////////////////////////////////////////
CRConClient* g_pRConClient = new CRConClient();
CRConClient* RCONClient()
{
	return g_pRConClient;
}