//===========================================================================//
//
// Purpose: Server/Client dual-stack socket utility class
//
//===========================================================================//

#include <tier1/NetAdr.h>
#include <tier2/socketcreator.h>
#ifndef NETCONSOLE
#include <engine/sys_utils.h>
#endif // !NETCONSOLE
#include <engine/net.h>

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CSocketCreator::CSocketCreator(void)
{
	m_hListenSocket = SOCKET_ERROR;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CSocketCreator::~CSocketCreator(void)
{
	DisconnectSockets();
}

//-----------------------------------------------------------------------------
// Purpose: accept new connections and walk open sockets and handle any incoming data
//-----------------------------------------------------------------------------
void CSocketCreator::RunFrame(void)
{
	if (IsListening())
	{
		ProcessAccept(); // handle any new connection requests.
	}
}

//-----------------------------------------------------------------------------
// Purpose: handle a new connection
//-----------------------------------------------------------------------------
void CSocketCreator::ProcessAccept(void)
{
	sockaddr_storage inClient{};
	int nLengthAddr = sizeof(inClient);
	SocketHandle_t newSocket = SocketHandle_t(::accept(SOCKET(m_hListenSocket), reinterpret_cast<sockaddr*>(&inClient), &nLengthAddr));
	if (newSocket == SOCKET_ERROR)
	{
		if (!IsSocketBlocking())
		{
			Error(eDLL_T::COMMON, NO_ERROR, "%s - Error: %s\n", __FUNCTION__, NET_ErrorString(WSAGetLastError()));
		}
		return;
	}

	if (!ConfigureSocket(newSocket, false))
	{
		DisconnectSocket(newSocket);
		return;
	}

	netadr_t netAdr;
	netAdr.SetFromSockadr(&inClient);

	OnSocketAccepted(newSocket, netAdr);
}

//-----------------------------------------------------------------------------
// Purpose: bind to a TCP port and accept incoming connections
// Input  : *netAdr - 
//			bDualStack - 
// Output : true on success, failed otherwise
//-----------------------------------------------------------------------------
bool CSocketCreator::CreateListenSocket(const netadr_t& netAdr, bool bDualStack)
{
	CloseListenSocket();
	m_hListenSocket = SocketHandle_t(::socket(PF_INET6, SOCK_STREAM, IPPROTO_TCP));

	if (m_hListenSocket != INVALID_SOCKET)
	{
		if (!ConfigureSocket(m_hListenSocket, bDualStack))
		{
			CloseListenSocket();
			return false;
		}

		sockaddr_storage sadr{};
		netAdr.ToSockadr(&sadr);

		int results = ::bind(m_hListenSocket, reinterpret_cast<sockaddr*>(&sadr), sizeof(sockaddr_in6));
		if (results == SOCKET_ERROR)
		{
			Warning(eDLL_T::COMMON, "Socket bind failed (%s)\n", NET_ErrorString(WSAGetLastError()));
			CloseListenSocket();

			return false;
		}

		results = ::listen(m_hListenSocket, SOCKET_TCP_MAX_ACCEPTS);
		if (results == SOCKET_ERROR)
		{
			Warning(eDLL_T::COMMON, "Socket listen failed (%s)\n", NET_ErrorString(WSAGetLastError()));
			CloseListenSocket();

			return false;
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: close an open rcon connection
//-----------------------------------------------------------------------------
void CSocketCreator::CloseListenSocket(void)
{
	if (m_hListenSocket != SOCKET_ERROR)
	{
		DisconnectSocket(m_hListenSocket);
		m_hListenSocket = SOCKET_ERROR;
	}
}

//-----------------------------------------------------------------------------
// Purpose: connect to the remote server
// Input  : *netAdr - 
//			bSingleSocket - 
// Output : accepted socket index, SOCKET_ERROR (-1) if failed
//-----------------------------------------------------------------------------
int CSocketCreator::ConnectSocket(const netadr_t& netAdr, bool bSingleSocket)
{
	if (bSingleSocket)
	{ // NOTE: Closing an accepted socket will re-index all the sockets with higher indices.
		CloseAllAcceptedSockets();
	}

	SocketHandle_t hSocket = SocketHandle_t(::socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP));
	if (hSocket == SOCKET_ERROR)
	{
		Warning(eDLL_T::COMMON, "Unable to create socket (%s)\n", NET_ErrorString(WSAGetLastError()));
		return SOCKET_ERROR;
	}

	if (!ConfigureSocket(hSocket))
	{
		DisconnectSocket(hSocket);
		return SOCKET_ERROR;
	}

	struct sockaddr_storage s{};
	netAdr.ToSockadr(&s);

	int results = ::connect(hSocket, reinterpret_cast<sockaddr*>(&s), sizeof(sockaddr_in6));
	if (results == SOCKET_ERROR)
	{
		if (!IsSocketBlocking())
		{
			Warning(eDLL_T::COMMON, "Socket connection failed (%s)\n", NET_ErrorString(WSAGetLastError()));

			DisconnectSocket(hSocket);
			return SOCKET_ERROR;
		}

		fd_set writefds{};
		timeval tv{};

		tv.tv_usec = 0;
		tv.tv_sec  = 1;

		FD_ZERO(&writefds);
		FD_SET(static_cast<u_int>(hSocket), &writefds);

		if (::select(hSocket + 1, NULL, &writefds, NULL, &tv) < 1) // block for at most 1 second.
		{
			Warning(eDLL_T::COMMON, "Socket connection timed out\n");
			DisconnectSocket(hSocket); // took too long to connect to, give up.

			return SOCKET_ERROR;
		}
	}

	// TODO: CRConClient check if connected.

	int nIndex = OnSocketAccepted(hSocket, netAdr);
	return nIndex;
}

//-----------------------------------------------------------------------------
// Purpose: closes specific open sockets (listen + accepted)
//-----------------------------------------------------------------------------
void CSocketCreator::DisconnectSocket(SocketHandle_t hSocket)
{
	Assert(hSocket != SOCKET_ERROR);
	if (::closesocket(hSocket) == SOCKET_ERROR)
	{
		Error(eDLL_T::COMMON, NO_ERROR, "Unable to close socket (%s)\n",
			NET_ErrorString(WSAGetLastError()));
	}
}

//-----------------------------------------------------------------------------
// Purpose: closes all open sockets (listen + accepted)
//-----------------------------------------------------------------------------
void CSocketCreator::DisconnectSockets(void)
{
	CloseListenSocket();
	CloseAllAcceptedSockets();
}

//-----------------------------------------------------------------------------
// Purpose: Configures a socket for use
// Input  : iSocket - 
//			bDualStack - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CSocketCreator::ConfigureSocket(SocketHandle_t hSocket, bool bDualStack /*= true*/)
{
	// Disable NAGLE as RCON cmds are small in size.
	int opt = 1;
	int ret = ::setsockopt(hSocket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char*>(&opt), sizeof(opt));
	if (ret == SOCKET_ERROR)
	{
		Warning(eDLL_T::COMMON, "Socket 'sockopt(%s)' failed (%s)\n", "TCP_NODELAY", NET_ErrorString(WSAGetLastError()));
		return false;
	}

	// Mark socket as reusable.
	opt = 1;
	ret = ::setsockopt(hSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char*>(&opt), sizeof(opt));
	if (ret == SOCKET_ERROR)
	{
		Warning(eDLL_T::COMMON, "Socket 'sockopt(%s)' failed (%s)\n", "SO_REUSEADDR", NET_ErrorString(WSAGetLastError()));
		return false;
	}

	if (bDualStack)
	{
		// Disable IPv6 only mode to enable dual stack.
		opt = 0;
		ret = ::setsockopt(hSocket, IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast<char*>(&opt), sizeof(opt));
		if (ret == SOCKET_ERROR)
		{
			Warning(eDLL_T::COMMON, "Socket 'sockopt(%s)' failed (%s)\n", "IPV6_V6ONLY", NET_ErrorString(WSAGetLastError()));
			return false;
		}
	}

	// Mark socket as non-blocking.
	opt = 1;
	ret = ::ioctlsocket(hSocket, FIONBIO, reinterpret_cast<u_long*>(&opt));
	if (ret == SOCKET_ERROR)
	{
		Warning(eDLL_T::COMMON, "Socket 'ioctl(%s)' failed (%s)\n", "FIONBIO", NET_ErrorString(WSAGetLastError()));
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: handles new TCP requests and puts them in accepted queue
// Input  : hSocket - 
//			*netAdr - 
// Output : accepted socket index, -1 if failed
//-----------------------------------------------------------------------------
int CSocketCreator::OnSocketAccepted(SocketHandle_t hSocket, const netadr_t& netAdr)
{
	AcceptedSocket_t newEntry(hSocket);
	newEntry.m_Address = netAdr;

	m_AcceptedSockets.AddToTail(newEntry);

	int nIndex = m_AcceptedSockets.Count() - 1;
	return nIndex;
}

//-----------------------------------------------------------------------------
// Purpose: close an accepted socket
// Input  : nIndex - 
//-----------------------------------------------------------------------------
void CSocketCreator::CloseAcceptedSocket(int nIndex)
{
	if (nIndex >= m_AcceptedSockets.Count())
	{
		Assert(0);
		return;
	}

	AcceptedSocket_t& connected = m_AcceptedSockets[nIndex];
	DisconnectSocket(connected.m_hSocket);

	m_AcceptedSockets.Remove(nIndex);
}

//-----------------------------------------------------------------------------
// Purpose: close all accepted sockets
//-----------------------------------------------------------------------------
void CSocketCreator::CloseAllAcceptedSockets(void)
{
	for (int i = 0; i < m_AcceptedSockets.Count(); ++i)
	{
		AcceptedSocket_t& connected = m_AcceptedSockets[i];
		DisconnectSocket(connected.m_hSocket);
	}
	m_AcceptedSockets.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the listening socket is created and listening
// Output : bool
//-----------------------------------------------------------------------------
bool CSocketCreator::IsListening(void) const
{
	return m_hListenSocket != SOCKET_ERROR;
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the socket would block because of the last socket command
// Output : bool
//-----------------------------------------------------------------------------
bool CSocketCreator::IsSocketBlocking(void) const
{
	return (WSAGetLastError() == WSAEWOULDBLOCK);
}

//-----------------------------------------------------------------------------
// Purpose: returns authorized socket count
// Output : int
//-----------------------------------------------------------------------------
int CSocketCreator::GetAuthorizedSocketCount(void) const
{
	int ret = 0;

	for (int i = 0; i < m_AcceptedSockets.Count(); ++i)
	{
		if (m_AcceptedSockets[i].m_Data.m_bAuthorized)
		{
			ret++;
		}
	}

	return ret;
}

//-----------------------------------------------------------------------------
// Purpose: returns accepted socket count
// Output : int
//-----------------------------------------------------------------------------
int CSocketCreator::GetAcceptedSocketCount(void) const
{
	return m_AcceptedSockets.Count();
}

//-----------------------------------------------------------------------------
// Purpose: returns accepted socket handle
// Input  : nIndex - 
// Output : SocketHandle_t
//-----------------------------------------------------------------------------
SocketHandle_t CSocketCreator::GetAcceptedSocketHandle(int nIndex) const
{
	Assert(nIndex >= 0 && nIndex < m_AcceptedSockets.Count());
	return m_AcceptedSockets[nIndex].m_hSocket;
}

//-----------------------------------------------------------------------------
// Purpose: returns accepted socket address
// Input  : nIndex - 
// Output : const netadr_t&
//-----------------------------------------------------------------------------
const netadr_t& CSocketCreator::GetAcceptedSocketAddress(int nIndex) const
{
	Assert(nIndex >= 0 && nIndex < m_AcceptedSockets.Count());
	return m_AcceptedSockets[nIndex].m_Address;
}

//-----------------------------------------------------------------------------
// Purpose: returns accepted socket data
// Input  : nIndex - 
// Output : CConnectedNetConsoleData*
//-----------------------------------------------------------------------------
CConnectedNetConsoleData& CSocketCreator::GetAcceptedSocketData(int nIndex)
{
	Assert(nIndex >= 0 && nIndex < m_AcceptedSockets.Count());
	return m_AcceptedSockets[nIndex].m_Data;
}

//-----------------------------------------------------------------------------
// Purpose: returns accepted socket data
// Input  : nIndex - 
// Output : CConnectedNetConsoleData*
//-----------------------------------------------------------------------------
const CConnectedNetConsoleData& CSocketCreator::GetAcceptedSocketData(int nIndex) const
{
	Assert(nIndex >= 0 && nIndex < m_AcceptedSockets.Count());
	return m_AcceptedSockets[nIndex].m_Data;
}
