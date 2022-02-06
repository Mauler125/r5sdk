//===========================================================================//
//
// Purpose: Server/Client dual-stack socket utility class
//
//===========================================================================//

#include <core/stdafx.h>
#include <tier1/NetAdr2.h>
#include <tier2/socketcreator.h>
#ifndef NETCONSOLE
#include <engine/sys_utils.h>
#endif // !NETCONSOLE
#include <engine/net.h>
#include <netconsole/netconsole.h>

// TODO [AMOS] IMPLEMENT 'Warning(...)' for every DevMsg spew here..

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
	int newSocket = accept(m_hListenSocket, reinterpret_cast<sockaddr*>(&inClient), &nLengthAddr);
	if (newSocket == -1)
	{
		if (!IsSocketBlocking())
		{
#ifndef NETCONSOLE
			DevMsg(eDLL_T::ENGINE, "Socket ProcessAccept Error: %s\n", NET_ErrorString(WSAGetLastError()));
#else
			printf("Socket ProcessAccept Error: %s\n", NET_ErrorString(WSAGetLastError()));
#endif // !NETCONSOLE
		}
		return;
	}

	if (!ConfigureListenSocket(newSocket))
	{
		closesocket(newSocket);
		return;
	}

	CNetAdr2 netAdr2;
	netAdr2.SetFromSockadr(&inClient);

	OnSocketAccepted(newSocket, netAdr2);
}

//-----------------------------------------------------------------------------
// Purpose: Configures a listen socket for use
//-----------------------------------------------------------------------------
bool CSocketCreator::ConfigureListenSocket(int iSocket)
{
	// Disable NAGLE as RCON cmds are small in size.
	int nodelay = 1;
	int v6only  = 0;
	u_long opt  = 1;

	setsockopt(iSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&nodelay, sizeof(nodelay));
	setsockopt(iSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&nodelay, sizeof(nodelay));
	setsockopt(iSocket, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&v6only, sizeof(v6only));

	int results = ioctlsocket(iSocket, FIONBIO, (u_long*)&opt); // Non-blocking.
	if (results == -1)
	{
#ifndef NETCONSOLE
		DevMsg(eDLL_T::ENGINE, "Socket accept 'ioctl(FIONBIO)' failed (%i)\n", WSAGetLastError());
#else
		printf("Socket accept 'ioctl(FIONBIO)' failed (%i)\n", WSAGetLastError());
#endif // !NETCONSOLE
		return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Configures a accepted socket for use
//-----------------------------------------------------------------------------
bool CSocketCreator::ConfigureConnectSocket(SocketHandle_t hSocket)
{
	int opt = 1;
	int ret = 0;

	ret = ioctlsocket(hSocket, FIONBIO, reinterpret_cast<u_long*>(&opt)); // Non-blocking
	if (ret == -1)
	{
#ifndef NETCONSOLE
		DevMsg(eDLL_T::ENGINE, "Socket ioctl(FIONBIO) failed (%s)\n", NET_ErrorString(WSAGetLastError()));
#else
		printf("Socket ioctl(FIONBIO) failed (%s)\n", NET_ErrorString(WSAGetLastError()));
#endif // !NETCONSOLE
		closesocket(hSocket);
		return false;
	}

	// Disable NAGLE as RCON cmds are small in size.
	int nodelay = 1;
	setsockopt(hSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&nodelay, sizeof(nodelay));

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: bind to a TCP port and accept incoming connections
// Input  : *netAdr2 - 
//			bListenOnAllInterfaces - 
// Output : true on success, failed otherwise
//-----------------------------------------------------------------------------
bool CSocketCreator::CreateListenSocket(const CNetAdr2& netAdr2, bool bListenOnAllInterfaces = false)
{
	CloseListenSocket();
	m_ListenAddress = netAdr2;
	m_hListenSocket = socket(PF_INET6, SOCK_STREAM, IPPROTO_TCP);

	if (m_hListenSocket != INVALID_SOCKET)
	{
		if (!ConfigureListenSocket(m_hListenSocket))
		{
			CloseListenSocket();
			return false;
		}

		sockaddr_storage sadr{};
		m_ListenAddress.ToSockadr(&sadr);

		int results = bind(m_hListenSocket, reinterpret_cast<sockaddr*>(&sadr), m_ListenAddress.GetSize());
		if (results == -1)
		{
#ifndef NETCONSOLE
			DevMsg(eDLL_T::ENGINE, "Socket bind failed (%s)\n", NET_ErrorString(WSAGetLastError()));
#else
			printf("Socket bind failed (%s)\n", NET_ErrorString(WSAGetLastError()));
#endif // !NETCONSOLE
			CloseListenSocket();
			return false;
		}

		results = listen(m_hListenSocket, SOCKET_TCP_MAX_ACCEPTS);
		if (results == -1)
		{
#ifndef NETCONSOLE
			DevMsg(eDLL_T::ENGINE, "Socket listen failed (%s)\n", NET_ErrorString(WSAGetLastError()));
#else
			printf("Socket listen failed (%s)\n", NET_ErrorString(WSAGetLastError()));
#endif // !NETCONSOLE
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
		closesocket(m_hListenSocket);
		m_hListenSocket = -1;
	}
}

//-----------------------------------------------------------------------------
// Purpose: connect to the remote server
// Input  : *netAdr2 - 
//			bSingleSocker - 
// Output : accepted socket index, SOCKET_ERROR (-1) if failed
//-----------------------------------------------------------------------------
int CSocketCreator::ConnectSocket(const CNetAdr2& netAdr2, bool bSingleSocket)
{
	if (bSingleSocket)
	{ // NOTE: Closing an accepted socket will re-index all the sockets with higher indices
		CloseAllAcceptedSockets();
	}

	SocketHandle_t hSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (hSocket == SOCKET_ERROR)
	{
#ifndef NETCONSOLE
		DevMsg(eDLL_T::ENGINE, "Unable to create socket (%s)\n", NET_ErrorString(WSAGetLastError()));
#else
		printf("Unable to create socket (%s)\n", NET_ErrorString(WSAGetLastError()));
#endif // !NETCONSOLE
		return SOCKET_ERROR;
	}

	if (!ConfigureConnectSocket(hSocket))
	{
		return SOCKET_ERROR;
	}

	struct sockaddr_storage s{};
	netAdr2.ToSockadr(&s);

	int results = connect(hSocket, reinterpret_cast<sockaddr*>(&s), sizeof(s));
	if (results == SOCKET_ERROR)
	{
		if (!IsSocketBlocking())
		{
#ifndef NETCONSOLE
			DevMsg(eDLL_T::ENGINE, "Socket connection failed (%s)\n", NET_ErrorString(WSAGetLastError()));
#else
			printf("Socket connection failed (%s)\n", NET_ErrorString(WSAGetLastError()));
#endif // !NETCONSOLE
			closesocket(hSocket);
			return SOCKET_ERROR;
		}

		fd_set writefds{};
		timeval tv{};

		tv.tv_usec = 0;
		tv.tv_sec  = 1;

		FD_ZERO(&writefds);
		FD_SET(static_cast<u_int>(hSocket), &writefds);

		if (select(hSocket + 1, NULL, &writefds, NULL, &tv) < 1) // block for at most 1 second
		{
			closesocket(hSocket); // took too long to connect to, give up
			return SOCKET_ERROR;
		}
	}

	// TODO: CRConClient check if connected.

	int nIndex = OnSocketAccepted(hSocket, netAdr2);
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
//			*netAdr2 - 
// Output : accepted socket index, -1 if failed
//-----------------------------------------------------------------------------
int CSocketCreator::OnSocketAccepted(SocketHandle_t hSocket, CNetAdr2 netAdr2)
{
	AcceptedSocket_t pNewEntry;

	pNewEntry.m_hSocket = hSocket;
	pNewEntry.m_Address = netAdr2;
	pNewEntry.m_pData   = new CConnectedNetConsoleData(hSocket);

	m_hAcceptedSockets.push_back(pNewEntry);

	int nIndex = (int)m_hAcceptedSockets.size() - 1;
	return nIndex;
}

//-----------------------------------------------------------------------------
// Purpose: close an accepted socket
// Input  : nIndex - 
//-----------------------------------------------------------------------------
void CSocketCreator::CloseAcceptedSocket(int nIndex)
{
	if (nIndex >= m_hAcceptedSockets.size())
	{
		return;
	}

	AcceptedSocket_t& connected = m_hAcceptedSockets[nIndex];
	closesocket(connected.m_hSocket);
	m_hAcceptedSockets.erase(m_hAcceptedSockets.begin() + nIndex);
}

//-----------------------------------------------------------------------------
// Purpose: close all accepted sockets
//-----------------------------------------------------------------------------
void CSocketCreator::CloseAllAcceptedSockets(void)
{
	int nCount = m_hAcceptedSockets.size();
	for (int i = 0; i < nCount; ++i)
	{
		AcceptedSocket_t& connected = m_hAcceptedSockets[i];
		closesocket(connected.m_hSocket);
	}
	m_hAcceptedSockets.clear();
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the listening socket is created and listening
//-----------------------------------------------------------------------------
bool CSocketCreator::IsListening(void) const
{
	return m_hListenSocket != -1;
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the socket would block because of the last socket command
//-----------------------------------------------------------------------------
bool CSocketCreator::IsSocketBlocking(void) const
{
	return (WSAGetLastError() == WSAEWOULDBLOCK);
}

//-----------------------------------------------------------------------------
// Purpose: returns accepted socket count
//-----------------------------------------------------------------------------
int CSocketCreator::GetAcceptedSocketCount(void) const
{
	return m_hAcceptedSockets.size();
}

//-----------------------------------------------------------------------------
// Purpose: returns accepted socket handle
//-----------------------------------------------------------------------------
SocketHandle_t CSocketCreator::GetAcceptedSocketHandle(int nIndex) const
{
	return m_hAcceptedSockets[nIndex].m_hSocket;
}

//-----------------------------------------------------------------------------
// Purpose: returns accepted socket address
//-----------------------------------------------------------------------------
const CNetAdr2& CSocketCreator::GetAcceptedSocketAddress(int nIndex) const
{
	return m_hAcceptedSockets[nIndex].m_Address;
}

//-----------------------------------------------------------------------------
// Purpose: returns accepted socket data
//-----------------------------------------------------------------------------
CConnectedNetConsoleData* CSocketCreator::GetAcceptedSocketData(int nIndex) const
{
	return m_hAcceptedSockets[nIndex].m_pData;
}
