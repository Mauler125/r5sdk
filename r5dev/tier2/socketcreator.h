#pragma once
#include "tier1/netadr2.h"
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

	bool CreateListenSocket(const CNetAdr2& netAdr2, bool bListenOnAllInterfaces);
	void CloseListenSocket(void);

	int ConnectSocket(const CNetAdr2& netAdr2, bool bSingleSocket);
	void DisconnectSocket(void);

	int OnSocketAccepted(SocketHandle_t hSocket, CNetAdr2 netAdr2);

	void CloseAcceptedSocket(int nIndex);
	void CloseAllAcceptedSockets(void);

	bool IsListening(void) const;
	bool IsSocketBlocking(void) const;

	int GetAcceptedSocketCount(void) const;
	SocketHandle_t GetAcceptedSocketHandle(int nIndex) const;
	const CNetAdr2& GetAcceptedSocketAddress(int nIndex) const;
	CConnectedNetConsoleData* GetAcceptedSocketData(int nIndex) const;

public:
	struct AcceptedSocket_t
	{
		SocketHandle_t            m_hSocket{};
		CNetAdr2                  m_Address{};
		CConnectedNetConsoleData* m_pData = nullptr;

		bool operator==(const AcceptedSocket_t& rhs) const { return (m_Address.CompareAdr(rhs.m_Address, false) == 0); }
	};

	std::vector<AcceptedSocket_t> m_hAcceptedSockets{};
	SocketHandle_t                m_hListenSocket   {}; // Used to accept connections.
	CNetAdr2                      m_ListenAddress   {}; // Address used to listen on.

private:
	enum
	{
		SOCKET_TCP_MAX_ACCEPTS = 2
	};
};
