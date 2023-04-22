//===========================================================================//
// 
// Purpose: Base rcon implementation.
// 
//===========================================================================//
#include "core/stdafx.h"
#include "base_rcon.h"
#include "engine/net.h"
#include "shared_rcon.h"

//-----------------------------------------------------------------------------
// Purpose: encode message to buffer
// Input  : *pMsg - 
//			*pMsgBuf - 
//			nMsgLen - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CNetConBase::Encode(google::protobuf::MessageLite* pMsg,
	char* pMsgBuf, const size_t nMsgLen) const
{
	return pMsg->SerializeToArray(pMsgBuf, int(nMsgLen));
}

//-----------------------------------------------------------------------------
// Purpose: decode message from buffer
// Input  : *pMsg - 
//			*pMsgBuf - 
//			nMsgLen - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CNetConBase::Decode(google::protobuf::MessageLite* pMsg,
	const char* pMsgBuf, const size_t nMsgLen) const
{
	return pMsg->ParseFromArray(pMsgBuf, int(nMsgLen));
}

//-----------------------------------------------------------------------------
// Purpose: connect to remote
// Input  : *pHostName - 
//			nPort - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CNetConBase::Connect(const char* pHostName, const int nPort)
{
	return CL_NetConConnect(this, pHostName, nPort);
}

//-----------------------------------------------------------------------------
// Purpose: send message to specific connected socket
// Input  : hSocket - 
//			*pMsgBuf - 
//			nMsgLen - 
// Output: true on success, false otherwise
//-----------------------------------------------------------------------------
bool CNetConBase::Send(const SocketHandle_t hSocket, const char* pMsgBuf,
	const int nMsgLen) const
{
	std::ostringstream sendbuf;
	const u_long nLen = htonl(u_long(nMsgLen));

	sendbuf.write(reinterpret_cast<const char*>(&nLen), sizeof(u_long));
	sendbuf.write(pMsgBuf, nMsgLen);

	int ret = ::send(hSocket, sendbuf.str().data(), int(sendbuf.str().size()),
		MSG_NOSIGNAL);

	return (ret != SOCKET_ERROR);
}

//-----------------------------------------------------------------------------
// Purpose: receive message
// Input  : *pData - 
//			nMaxLen - 
// Output: true on success, false otherwise
//-----------------------------------------------------------------------------
void CNetConBase::Recv(CConnectedNetConsoleData* pData, const int nMaxLen)
{
	if (!pData)
	{
		Error(eDLL_T::ENGINE, NO_ERROR, "RCON Cmd: invalid input data\n");
		Assert(0);
		return;
	}

	static char szRecvBuf[1024];

	{//////////////////////////////////////////////
		const int nPendingLen = ::recv(pData->m_hSocket, szRecvBuf, sizeof(char), MSG_PEEK);
		if (nPendingLen == SOCKET_ERROR && m_Socket.IsSocketBlocking())
		{
			return;
		}
		if (nPendingLen <= 0) // EOF or error.
		{
			Disconnect("unexpected EOF or error");
			return;
		}
	}//////////////////////////////////////////////

	int nReadLen = 0; // Find out how much we have to read.
	int iResult = ::ioctlsocket(pData->m_hSocket, FIONREAD, reinterpret_cast<u_long*>(&nReadLen));

	if (iResult == SOCKET_ERROR)
	{
		Error(eDLL_T::ENGINE, NO_ERROR, "RCON Cmd: ioctl(%s) error (%s)\n", "FIONREAD", NET_ErrorString(WSAGetLastError()));
		return;
	}

	bool bSuccess = true;

	while (nReadLen > 0)
	{
		const int nRecvLen = ::recv(pData->m_hSocket, szRecvBuf, MIN(sizeof(szRecvBuf), nReadLen), MSG_NOSIGNAL);
		if (nRecvLen == 0) // Socket was closed.
		{
			Disconnect("socket closed unexpectedly");
			break;
		}
		if (nRecvLen < 0 && !m_Socket.IsSocketBlocking())
		{
			Error(eDLL_T::ENGINE, NO_ERROR, "RCON Cmd: recv error (%s)\n", NET_ErrorString(WSAGetLastError()));
			break;
		}

		nReadLen -= nRecvLen; // Process what we've got.
		if (!ProcessBuffer(pData, szRecvBuf, nRecvLen, nMaxLen) && bSuccess)
		{
			bSuccess = false;
		}
	}

	return;
}

//-----------------------------------------------------------------------------
// Purpose: parses input response buffer using length-prefix framing
// Input  : *pRecvBuf - 
//			nRecvLen - 
//			*pData - 
// Output: true on success, false otherwise
//-----------------------------------------------------------------------------
bool CNetConBase::ProcessBuffer(CConnectedNetConsoleData* pData, 
	const char* pRecvBuf, int nRecvLen, const int nMaxLen)
{
	bool bSuccess = true;

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
				if (!ProcessMessage(
					reinterpret_cast<const char*>(pData->m_RecvBuffer.data()), pData->m_nPayloadLen)
					&& bSuccess)
				{
					bSuccess = false;
				}

				pData->m_nPayloadLen = 0;
				pData->m_nPayloadRead = 0;
			}
		}
		else if (pData->m_nPayloadRead+1 <= sizeof(int)) // Read size field.
		{
			pData->m_RecvBuffer[pData->m_nPayloadRead++] = *pRecvBuf;

			pRecvBuf++;
			nRecvLen--;
		}
		else // Build prefix.
		{
			pData->m_nPayloadLen = int(ntohl(*reinterpret_cast<u_long*>(&pData->m_RecvBuffer[0])));
			pData->m_nPayloadRead = 0;

			if (!pData->m_bAuthorized && nMaxLen > -1)
			{
				if (pData->m_nPayloadLen > nMaxLen)
				{
					Disconnect("overflow"); // Sending large messages while not authenticated.
					return false;
				}
			}

			if (pData->m_nPayloadLen < 0 ||
				pData->m_nPayloadLen > pData->m_RecvBuffer.max_size())
			{
				Error(eDLL_T::ENGINE, NO_ERROR, "RCON Cmd: sync error (%d)\n", pData->m_nPayloadLen);
				Disconnect("desync"); // Out of sync (irrecoverable).

				return false;
			}
			else
			{
				pData->m_RecvBuffer.resize(pData->m_nPayloadLen);
			}
		}
	}

	return bSuccess;
}
