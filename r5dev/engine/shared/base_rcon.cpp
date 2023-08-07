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
// Purpose: parses input response buffer using length-prefix framing
// Input  : &data - 
//			*pRecvBuf - 
//			nRecvLen - 
//			nMaxLen - 
// Output: true on success, false otherwise
//-----------------------------------------------------------------------------
bool CNetConBase::ProcessBuffer(CConnectedNetConsoleData& data, 
	const char* pRecvBuf, int nRecvLen, const int nMaxLen)
{
	bool bSuccess = true;

	while (nRecvLen > 0)
	{
		if (data.m_nPayloadLen)
		{
			if (data.m_nPayloadRead < data.m_nPayloadLen)
			{
				data.m_RecvBuffer[data.m_nPayloadRead++] = *pRecvBuf;

				pRecvBuf++;
				nRecvLen--;
			}
			if (data.m_nPayloadRead == data.m_nPayloadLen)
			{
				if (!ProcessMessage(
					reinterpret_cast<const char*>(data.m_RecvBuffer.data()), data.m_nPayloadLen)
					&& bSuccess)
				{
					bSuccess = false;
				}

				data.m_nPayloadLen = 0;
				data.m_nPayloadRead = 0;
			}
		}
		else if (data.m_nPayloadRead+1 <= sizeof(int)) // Read size field.
		{
			data.m_RecvBuffer[data.m_nPayloadRead++] = *pRecvBuf;

			pRecvBuf++;
			nRecvLen--;
		}
		else // Build prefix.
		{
			data.m_nPayloadLen = int(ntohl(*reinterpret_cast<u_long*>(&data.m_RecvBuffer[0])));
			data.m_nPayloadRead = 0;

			if (!data.m_bAuthorized && nMaxLen > -1)
			{
				if (data.m_nPayloadLen > nMaxLen)
				{
					Disconnect("overflow"); // Sending large messages while not authenticated.
					return false;
				}
			}

			if (data.m_nPayloadLen < 0 ||
				data.m_nPayloadLen > data.m_RecvBuffer.max_size())
			{
				Error(eDLL_T::ENGINE, NO_ERROR, "RCON Cmd: sync error (%d)\n", data.m_nPayloadLen);
				Disconnect("desync"); // Out of sync (irrecoverable).

				return false;
			}
			else
			{
				data.m_RecvBuffer.resize(data.m_nPayloadLen);
			}
		}
	}

	return bSuccess;
}

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
// Input  : &data - 
//			nMaxLen - 
// Output: true on success, false otherwise
//-----------------------------------------------------------------------------
void CNetConBase::Recv(CConnectedNetConsoleData& data, const int nMaxLen)
{
	static char szRecvBuf[1024];

	{//////////////////////////////////////////////
		const int nPendingLen = ::recv(data.m_hSocket, szRecvBuf, sizeof(char), MSG_PEEK);
		if (nPendingLen == SOCKET_ERROR && m_Socket.IsSocketBlocking())
		{
			return;
		}
		else if (nPendingLen == 0) // Socket was closed.
		{
			Disconnect("remote closed socket");
			return;
		}
		else if (nPendingLen < 0)
		{
			Disconnect("socket closed unexpectedly");
			return;
		}
	}//////////////////////////////////////////////

	int nReadLen = 0; // Find out how much we have to read.
	int iResult = ::ioctlsocket(data.m_hSocket, FIONREAD, reinterpret_cast<u_long*>(&nReadLen));

	if (iResult == SOCKET_ERROR)
	{
		Error(eDLL_T::ENGINE, NO_ERROR, "RCON Cmd: ioctl(%s) error (%s)\n", "FIONREAD", NET_ErrorString(WSAGetLastError()));
		return;
	}

	while (nReadLen > 0)
	{
		const int nRecvLen = ::recv(data.m_hSocket, szRecvBuf, MIN(sizeof(szRecvBuf), nReadLen), MSG_NOSIGNAL);
		if (nRecvLen == 0) // Socket was closed.
		{
			Disconnect("socket closed");
			break;
		}
		if (nRecvLen < 0 && !m_Socket.IsSocketBlocking())
		{
			Error(eDLL_T::ENGINE, NO_ERROR, "RCON Cmd: recv error (%s)\n", NET_ErrorString(WSAGetLastError()));
			break;
		}

		nReadLen -= nRecvLen; // Process what we've got.
		ProcessBuffer(data, szRecvBuf, nRecvLen, nMaxLen);
	}

	return;
}
