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
#include "vscript/languages/squirrel_re/include/sqvm.h"
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
	Disconnect("shutdown");
}

//-----------------------------------------------------------------------------
// Purpose: client rcon main processing loop
//-----------------------------------------------------------------------------
void CRConClient::RunFrame(void)
{
	if (IsInitialized() && IsConnected())
	{
		CConnectedNetConsoleData* pData = GetData();
		Assert(pData != nullptr);

		if (pData)
		{
			Recv(*pData);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: disconnect from current session
//-----------------------------------------------------------------------------
void CRConClient::Disconnect(const char* szReason)
{
	if (IsConnected())
	{
		if (!szReason)
		{
			szReason = "unknown reason";
		}

		Msg(eDLL_T::CLIENT, "Disconnect: (%s)\n", szReason);
		m_Socket.CloseAcceptedSocket(0);
	}
}

//-----------------------------------------------------------------------------
// Purpose: processes received message
// Input  : *pMsgBug - 
//			nMsgLen - 
//-----------------------------------------------------------------------------
bool CRConClient::ProcessMessage(const char* pMsgBuf, const int nMsgLen)
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
			const int i = atoi(response.responseval().c_str());

			// '!i' means we are marked 'input only' on the rcon server.
			if (!i && ShouldReceive())
			{
				RequestConsoleLog(true);
			}
		}

		Msg(eDLL_T::NETCON, "%s", response.responsemsg().c_str());
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
// Purpose: request the rcon server to enable/disable sending logs to us 
// Input  : bWantLog - 
//-----------------------------------------------------------------------------
void CRConClient::RequestConsoleLog(const bool bWantLog)
{
	// If 'IsRemoteLocal()' returns true, and you called this with 'bWantLog'
	// true, you caused a bug! It means the server address and port are equal
	// to the global netadr singleton, which ultimately means we are running on
	// a listen server. Listen server's already log to the same console,
	// sending logs will cause the print func to get called recursively forever.
	Assert(!(bWantLog && IsRemoteLocal()));

	const char* szEnable = bWantLog ? "1" : "0";
	const SocketHandle_t hSocket = GetSocket();

	vector<char> vecMsg;
	bool ret = Serialize(vecMsg, "", szEnable, cl_rcon::request_t::SERVERDATA_REQUEST_SEND_CONSOLE_LOG);

	if (ret && !Send(hSocket, vecMsg.data(), int(vecMsg.size())))
	{
		Error(eDLL_T::CLIENT, NO_ERROR, "Failed to send RCON message: (%s)\n", "SOCKET_ERROR");
	}
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
CConnectedNetConsoleData* CRConClient::GetData(void)
{
	return SH_GetNetConData(this, 0);
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
// Purpose: returns whether or not we should receive logs from the server
// Output : SOCKET_ERROR (-1) on failure
//-----------------------------------------------------------------------------
bool CRConClient::ShouldReceive(void)
{
	return (!IsRemoteLocal() && !cl_rcon_inputonly->GetBool());
}

//-----------------------------------------------------------------------------
// Purpose: returns whether the rcon server is actually our own listen server
//-----------------------------------------------------------------------------
bool CRConClient::IsRemoteLocal(void)
{
	return (g_pNetAdr->ComparePort(m_Address) && g_pNetAdr->CompareAdr(m_Address));
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
CRConClient* g_RCONClient(new CRConClient());
CRConClient* RCONClient() // Singleton RCON Client.
{
	return g_RCONClient;
}
