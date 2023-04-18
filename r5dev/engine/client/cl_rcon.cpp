//===========================================================================//
// 
// Purpose: Implementation of the rcon client.
// 
//===========================================================================//

#include "core/stdafx.h"
#include "tier1/cmd.h"
#include "tier1/cvar.h"
#include "protoc/sv_rcon.pb.h"
#include "protoc/cl_rcon.pb.h"
#include "engine/client/cl_rcon.h"
#include "engine/shared/shared_rcon.h"
#include "engine/net.h"
#include "squirrel/sqvm.h"
#include "common/igameserverdata.h"


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CRConClient::CRConClient()
	: m_bInitialized(false)
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CRConClient::~CRConClient(void)
{
}

//-----------------------------------------------------------------------------
// Purpose: NETCON systems init
//-----------------------------------------------------------------------------
void CRConClient::Init(void)
{
	m_bInitialized = true;
}

//-----------------------------------------------------------------------------
// Purpose: NETCON systems shutdown
//-----------------------------------------------------------------------------
void CRConClient::Shutdown(void)
{
	Disconnect();
}

//-----------------------------------------------------------------------------
// Purpose: client rcon main processing loop
//-----------------------------------------------------------------------------
void CRConClient::RunFrame(void)
{
	if (IsInitialized() && IsConnected())
	{
		CConnectedNetConsoleData* pData = m_Socket.GetAcceptedSocketData(0);
		Recv(pData);
	}
}

//-----------------------------------------------------------------------------
// Purpose: disconnect from current session
//-----------------------------------------------------------------------------
void CRConClient::Disconnect(const char* szReason)
{
	if (IsConnected())
	{
		if (szReason)
		{
			DevMsg(eDLL_T::CLIENT, "%s", szReason);
		}

		m_Socket.CloseAcceptedSocket(0);
	}
}

//-----------------------------------------------------------------------------
// Purpose: processes received message
// Input  : *pMsgBug - 
//			nMsgLen - 
//-----------------------------------------------------------------------------
bool CRConClient::ProcessMessage(const char* pMsgBuf, int nMsgLen)
{
	sv_rcon::response response;
	bool bSuccess = Decode(&response, pMsgBuf, nMsgLen);

	if (!bSuccess)
	{
		Error(eDLL_T::CLIENT, NO_ERROR, "Failed to decode RCON buffer\n");
		return false;
	}

	switch (response.responsetype())
	{
	case sv_rcon::response_t::SERVERDATA_RESPONSE_AUTH:
	{
		if (!response.responseval().empty())
		{
			const long i = strtol(response.responseval().c_str(), NULL, NULL);
			const bool bLocalHost = (g_pNetAdr->ComparePort(m_Address) && g_pNetAdr->CompareAdr(m_Address));
			const SocketHandle_t hSocket = GetSocket();

			if (!i) // sv_rcon_sendlogs is not set.
			{
				if (!bLocalHost && cl_rcon_request_sendlogs->GetBool())
				{
					vector<char> vecMsg;
					bool ret = Serialize(vecMsg, "", "1", cl_rcon::request_t::SERVERDATA_REQUEST_SEND_CONSOLE_LOG);

					if (ret && !Send(hSocket, vecMsg.data(), int(vecMsg.size())))
					{
						Error(eDLL_T::CLIENT, NO_ERROR, "Failed to send RCON message: (%s)\n", "SOCKET_ERROR");
					}
				}
			}
			else if (bLocalHost)
			{
				// Don't send logs to local host, it already gets logged to the same console.
				vector<char> vecMsg;
				bool ret = Serialize(vecMsg, "", "0", cl_rcon::request_t::SERVERDATA_REQUEST_SEND_CONSOLE_LOG);

				if (ret && !Send(hSocket, vecMsg.data(), int(vecMsg.size())))
				{
					Error(eDLL_T::CLIENT, NO_ERROR, "Failed to send RCON message: (%s)\n", "SOCKET_ERROR");
				}
			}
		}

		DevMsg(eDLL_T::NETCON, "%s", response.responsemsg().c_str());
		break;
	}
	case sv_rcon::response_t::SERVERDATA_RESPONSE_CONSOLE_LOG:
	{
		NetMsg(static_cast<LogType_t>(response.messagetype()), 
			static_cast<eDLL_T>(response.messageid()),
			response.responseval().c_str(), "%s", response.responsemsg().c_str());
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
// Purpose: serializes input
// Input  : *svReqBuf - 
//			*svReqVal - 
//			request_t - 
// Output : serialized results as string
//-----------------------------------------------------------------------------
bool CRConClient::Serialize(vector<char>& vecBuf, const char* szReqBuf,
	const char* szReqVal, const cl_rcon::request_t requestType) const
{
	return CL_NetConSerialize(this, vecBuf, szReqBuf, szReqVal, requestType);
}

//-----------------------------------------------------------------------------
// Purpose: retrieves the remote socket
// Output : SOCKET_ERROR (-1) on failure
//-----------------------------------------------------------------------------
SocketHandle_t CRConClient::GetSocket(void)
{
	return SH_GetNetConSocketHandle(this, 0);
}


//-----------------------------------------------------------------------------
// Purpose: checks if client rcon is initialized
//-----------------------------------------------------------------------------
bool CRConClient::IsInitialized(void) const
{
	return m_bInitialized;
}

//-----------------------------------------------------------------------------
// Purpose: returns whether the rcon client is connected
//-----------------------------------------------------------------------------
bool CRConClient::IsConnected(void)
{
	return (GetSocket() != SOCKET_ERROR);
}

///////////////////////////////////////////////////////////////////////////////
CRConClient g_RCONClient;
CRConClient* RCONClient() // Singleton RCON Client.
{
	return &g_RCONClient;
}