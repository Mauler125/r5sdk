//===========================================================================//
//
// Purpose: Server/Client dual-stack socket utility class
//
//===========================================================================//

#include <core/stdafx.h>
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
	m_hListenSocket = -1;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CSocketCreator::~CSocketCreator(void)
{
	DisconnectSocket();
}

//-----------------------------------------------------------------------------
// Purpose: accept new connections and walk open sockets and handle any incoming data
//-----------------------------------------------------------------------------
void CSocketCreator::RunFrame(void)
{
	if (IsListening())
	{
		ProcessAccept(); // handle any new connection requests
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
	if (newSocket == -1)
	{
		if (!IsSocketBlocking())
		{
			Error(eDLL_T::ENGINE, NO_ERROR, "%s - Error: %s\n", __FUNCTION__, NET_ErrorString(WSAGetLastError()));
		}
		return;
	}

	if (!ConfigureListenSocket(newSocket))
	{
		::closesocket(newSocket);
		return;
	}

	netadr_t netAdr;
	netAdr.SetFromSockadr(&inClient);

	OnSocketAccepted(newSocket, netAdr);
}

//-----------------------------------------------------------------------------
// Purpose: Configures a listen socket for use
// Input  : iSocket - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CSocketCreator::ConfigureListenSocket(int iSocket)
{
	// Disable NAGLE as RCON cmds are small in size.
	int nodelay = 1;
	int v6only  = 0;
	u_long opt  = 1;

	::setsockopt(iSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&nodelay, sizeof(nodelay));
	::setsockopt(iSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&nodelay, sizeof(nodelay));
	::setsockopt(iSocket, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&v6only, sizeof(v6only));

	int results = ::ioctlsocket(iSocket, FIONBIO, (u_long*)&opt); // Non-blocking.
	if (results == -1)
	{
		Warning(eDLL_T::ENGINE, "Socket accept 'ioctl(%s)' failed (%s)\n", "FIONBIO", NET_ErrorString(WSAGetLastError()));
		return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Configures a accepted socket for use
// Input  : hSocket - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CSocketCreator::ConfigureConnectSocket(SocketHandle_t hSocket)
{
	int opt = 1;
	int ret = 0;

	ret = ::ioctlsocket(hSocket, FIONBIO, reinterpret_cast<u_long*>(&opt)); // Non-blocking
	if (ret == -1)
	{
		Warning(eDLL_T::ENGINE, "Socket ioctl(%s) failed (%s)\n", "FIONBIO", NET_ErrorString(WSAGetLastError()));
		::closesocket(hSocket);
		return false;
	}

	// Disable NAGLE as RCON cmds are small in size.
	int nodelay = 1;
	::setsockopt(hSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&nodelay, sizeof(nodelay));

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: bind to a TCP port and accept incoming connections
// Input  : *netAdr - 
//			bListenOnAllInterfaces - 
// Output : true on success, failed otherwise
//-----------------------------------------------------------------------------
bool CSocketCreator::CreateListenSocket(const netadr_t& netAdr, bool bListenOnAllInterfaces)
{
	CloseListenSocket();
	m_ListenAddress = netAdr;
	m_hListenSocket = SocketHandle_t(::socket(PF_INET6, SOCK_STREAM, IPPROTO_TCP));

	if (m_hListenSocket != INVALID_SOCKET)
	{
		if (!ConfigureListenSocket(m_hListenSocket))
		{
			CloseListenSocket();
			return false;
		}

		sockaddr_storage sadr{};
		m_ListenAddress.ToSockadr(&sadr);

		int results = ::bind(m_hListenSocket, reinterpret_cast<sockaddr*>(&sadr), sizeof(sockaddr_in6));
		if (results == -1)
		{
			Warning(eDLL_T::ENGINE, "Socket bind failed (%s)\n", NET_ErrorString(WSAGetLastError()));
			CloseListenSocket();

			return false;
		}

		results = ::listen(m_hListenSocket, SOCKET_TCP_MAX_ACCEPTS);
		if (results == -1)
		{
			Warning(eDLL_T::ENGINE, "Socket listen failed (%s)\n", NET_ErrorString(WSAGetLastError()));
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
	if (m_hListenSocket != -1)
	{
		::closesocket(m_hListenSocket);
		m_hListenSocket = -1;
	}
}

//-----------------------------------------------------------------------------
// Purpose: connect to the remote server
// Input  : *netAdr - 
//			bSingleSocker - 
// Output : accepted socket index, SOCKET_ERROR (-1) if failed
//-----------------------------------------------------------------------------
int CSocketCreator::ConnectSocket(const netadr_t& netAdr, bool bSingleSocket)
{
	if (bSingleSocket)
	{ // NOTE: Closing an accepted socket will re-index all the sockets with higher indices
		CloseAllAcceptedSockets();
	}

	SocketHandle_t hSocket = SocketHandle_t(::socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP));
	if (hSocket == SOCKET_ERROR)
	{
		Warning(eDLL_T::ENGINE, "Unable to create socket (%s)\n", NET_ErrorString(WSAGetLastError()));
		return SOCKET_ERROR;
	}

	if (!ConfigureConnectSocket(hSocket))
	{
		return SOCKET_ERROR;
	}

	struct sockaddr_storage s{};
	netAdr.ToSockadr(&s);

	int results = ::connect(hSocket, reinterpret_cast<sockaddr*>(&s), sizeof(sockaddr_in6));
	if (results == SOCKET_ERROR)
	{
		if (!IsSocketBlocking())
		{
			Warning(eDLL_T::ENGINE, "Socket connection failed (%s)\n", NET_ErrorString(WSAGetLastError()));

			::closesocket(hSocket);
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
			::closesocket(hSocket); // took too long to connect to, give up.
			return SOCKET_ERROR;
		}
	}

	// TODO: CRConClient check if connected.

	int nIndex = OnSocketAccepted(hSocket, netAdr);
	return nIndex;
}

//-----------------------------------------------------------------------------
// Purpose: closes all open sockets (listen + accepted)
//-----------------------------------------------------------------------------
void CSocketCreator::DisconnectSocket(void)
{
	CloseListenSocket();
	CloseAllAcceptedSockets();
}

//-----------------------------------------------------------------------------
// Purpose: handles new TCP requests and puts them in accepted queue
// Input  : hSocket - 
//			*netAdr - 
// Output : accepted socket index, -1 if failed
//-----------------------------------------------------------------------------
int CSocketCreator::OnSocketAccepted(SocketHandle_t hSocket, const netadr_t& netAdr)
{
	AcceptedSocket_t newEntry;

	newEntry.m_hSocket = hSocket;
	newEntry.m_Address = netAdr;
	newEntry.m_pData   = new CConnectedNetConsoleData(hSocket);

	m_hAcceptedSockets.push_back(newEntry);

	int nIndex = static_cast<int>(m_hAcceptedSockets.size()) - 1;
	return nIndex;
}

//-----------------------------------------------------------------------------
// Purpose: close an accepted socket
// Input  : nIndex - 
//-----------------------------------------------------------------------------
void CSocketCreator::CloseAcceptedSocket(int nIndex)
{
	if (nIndex >= int(m_hAcceptedSockets.size()))
	{
		return;
	}

	AcceptedSocket_t& connected = m_hAcceptedSockets[nIndex];
	::closesocket(connected.m_hSocket);
	delete connected.m_pData;

	m_hAcceptedSockets.erase(m_hAcceptedSockets.begin() + nIndex);
}

//-----------------------------------------------------------------------------
// Purpose: close all accepted sockets
//-----------------------------------------------------------------------------
void CSocketCreator::CloseAllAcceptedSockets(void)
{
	for (size_t i = 0; i < m_hAcceptedSockets.size(); ++i)
	{
		AcceptedSocket_t& connected = m_hAcceptedSockets[i];
		::closesocket(connected.m_hSocket);

		delete connected.m_pData;
	}
	m_hAcceptedSockets.clear();
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the listening socket is created and listening
// Output : bool
//-----------------------------------------------------------------------------
bool CSocketCreator::IsListening(void) const
{
	return m_hListenSocket != -1;
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

	for (size_t i = 0; i < m_hAcceptedSockets.size(); ++i)
	{
		if (m_hAcceptedSockets[i].m_pData->m_bAuthorized)
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
	return static_cast<int>(m_hAcceptedSockets.size());
}

//-----------------------------------------------------------------------------
// Purpose: returns accepted socket handle
// Input  : nIndex - 
// Output : SocketHandle_t
//-----------------------------------------------------------------------------
SocketHandle_t CSocketCreator::GetAcceptedSocketHandle(int nIndex) const
{
	return m_hAcceptedSockets[nIndex].m_hSocket;
}

//-----------------------------------------------------------------------------
// Purpose: returns accepted socket address
// Input  : nIndex - 
// Output : const netadr_t&
//-----------------------------------------------------------------------------
const netadr_t& CSocketCreator::GetAcceptedSocketAddress(int nIndex) const
{
	return m_hAcceptedSockets[nIndex].m_Address;
}

//-----------------------------------------------------------------------------
// Purpose: returns accepted socket data
// Input  : nIndex - 
// Output : CConnectedNetConsoleData*
//-----------------------------------------------------------------------------
CConnectedNetConsoleData* CSocketCreator::GetAcceptedSocketData(int nIndex) const
{
	return m_hAcceptedSockets[nIndex].m_pData;
}
