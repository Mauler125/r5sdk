//===========================================================================//
// 
// Purpose: Shared rcon utilities.
// 
//===========================================================================//
#include "core/stdafx.h"
#include "base_rcon.h"
#include "shared_rcon.h"

//-----------------------------------------------------------------------------
// Purpose: serialize message to vector
// Input  : *pBase - 
//			&vecBuf - 
//			*szReqBuf - 
//			*szReqVal - 
//			*requestType - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CL_NetConSerialize(const CNetConBase* pBase, vector<char>& vecBuf, const char* szReqBuf,
	const char* szReqVal, const cl_rcon::request_t requestType)
{
	cl_rcon::request request;

	request.set_messageid(-1);
	request.set_requesttype(requestType);
	request.set_requestmsg(szReqBuf);
	request.set_requestval(szReqVal);

	const size_t msgLen = request.ByteSizeLong();
	vecBuf.resize(msgLen);

	if (!pBase->Encode(&request, &vecBuf[0], msgLen))
	{
		Error(eDLL_T::CLIENT, NO_ERROR, "Failed to encode RCON buffer\n");
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: attempt to connect to remote
// Input  : *pBase - 
//			*pHostAdr - 
//			nHostPort - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CL_NetConConnect(CNetConBase* pBase, const char* pHostAdr, const int nHostPort)
{
	string svLocalHost;
	const bool bValidSocket = nHostPort != SOCKET_ERROR;

	if (bValidSocket && strcmp(pHostAdr, "localhost") == 0)
	{
		char szHostName[512];
		if (!gethostname(szHostName, sizeof(szHostName)))
		{
			svLocalHost = Format("[%s]:%i", szHostName, nHostPort);
			pHostAdr = svLocalHost.c_str();
		}
	}

	CNetAdr* pNetAdr = pBase->GetNetAddress();
	if (!pNetAdr->SetFromString(pHostAdr, true))
	{
		Error(eDLL_T::CLIENT, NO_ERROR, "Failed to set RCON address: %s\n", pHostAdr);
		return false;
	}

	// Pass 'SOCKET_ERROR' if you want to set port from address string instead.
	if (bValidSocket)
	{
		pNetAdr->SetPort(htons(u_short(nHostPort)));
	}

	CSocketCreator* pCreator = pBase->GetSocketCreator();
	if (pCreator->ConnectSocket(*pNetAdr, true) == SOCKET_ERROR)
	{
		return false;
	}

	Msg(eDLL_T::CLIENT, "Connected to: %s\n", pNetAdr->ToString());
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: gets the netconsole data
// Input  : *pBase - 
//			iSocket - 
// Output : nullptr on failure
//-----------------------------------------------------------------------------
CConnectedNetConsoleData* SH_GetNetConData(CNetConBase* pBase, const int iSocket)
{
	CSocketCreator* pCreator = pBase->GetSocketCreator();
	Assert(iSocket >= 0 && (pCreator->GetAcceptedSocketCount() == 0
		|| iSocket < pCreator->GetAcceptedSocketCount()));

	if (!pCreator->GetAcceptedSocketCount())
	{
		return nullptr;
	}

	return &pCreator->GetAcceptedSocketData(iSocket);
}

//-----------------------------------------------------------------------------
// Purpose: gets the netconsole socket
// Input  : *pBase - 
//			iSocket - 
// Output : SOCKET_ERROR (-1) on failure
//-----------------------------------------------------------------------------
SocketHandle_t SH_GetNetConSocketHandle(CNetConBase* pBase, const int iSocket)
{
	const CConnectedNetConsoleData* pData = SH_GetNetConData(pBase, iSocket);
	if (!pData)
	{
		return SOCKET_ERROR;
	}

	return pData->m_hSocket;
}
