#pragma once
#include "tier1/netadr.h"
#include "common/igameserverdata.h"

//-----------------------------------------------------------------------------
// Purpose: container class to handle network streams
//-----------------------------------------------------------------------------
class CSocketCreator
{
public:
	CSocketCreator(void);
	~CSocketCreator(void);

	void RunFrame(void);
	void ProcessAccept(void);

	bool ConfigureListenSocket(int iSocket);
	bool ConfigureConnectSocket(SocketHandle_t hSocket);

	bool CreateListenSocket(const netadr_t& netAdr, bool bListenOnAllInterfaces = false);
	void CloseListenSocket(void);

	int ConnectSocket(const netadr_t& netAdr, bool bSingleSocket);
	void DisconnectSocket(void);

	int OnSocketAccepted(SocketHandle_t hSocket, const netadr_t& netAdr);

	void CloseAcceptedSocket(int nIndex);
	void CloseAllAcceptedSockets(void);

	bool IsListening(void) const;
	bool IsSocketBlocking(void) const;

	int GetAcceptedSocketCount(void) const;
	SocketHandle_t GetAcceptedSocketHandle(int nIndex) const;
	const netadr_t& GetAcceptedSocketAddress(int nIndex) const;
	CConnectedNetConsoleData* GetAcceptedSocketData(int nIndex) const;

public:
	struct AcceptedSocket_t
	{
		SocketHandle_t            m_hSocket{};
		netadr_t                  m_Address{};
		CConnectedNetConsoleData* m_pData = nullptr;
	};

	std::vector<AcceptedSocket_t> m_hAcceptedSockets;
	SocketHandle_t                m_hListenSocket; // Used to accept connections.
	netadr_t                      m_ListenAddress; // Address used to listen on.

private:
	enum
	{
		SOCKET_TCP_MAX_ACCEPTS = 2
	};
};
