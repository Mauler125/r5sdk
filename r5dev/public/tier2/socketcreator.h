#pragma once
#include "tier1/NetAdr.h"
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

	bool CreateListenSocket(const netadr_t& netAdr, bool bDualStack = true);
	void CloseListenSocket(void);

	int ConnectSocket(const netadr_t& netAdr, bool bSingleSocket);
	void DisconnectSocket(SocketHandle_t hSocket);
	void DisconnectSockets(void);

	bool ConfigureSocket(SocketHandle_t hSocket, bool bDualStack = true);
	int OnSocketAccepted(SocketHandle_t hSocket, const netadr_t& netAdr);

	void CloseAcceptedSocket(int nIndex);
	void CloseAllAcceptedSockets(void);

	bool IsListening(void) const;
	bool IsSocketBlocking(void) const;

	int GetAuthorizedSocketCount(void) const;
	int GetAcceptedSocketCount(void) const;

	SocketHandle_t GetAcceptedSocketHandle(int nIndex) const;
	const netadr_t& GetAcceptedSocketAddress(int nIndex) const;
	CConnectedNetConsoleData& GetAcceptedSocketData(int nIndex);
	const CConnectedNetConsoleData& GetAcceptedSocketData(int nIndex) const;

public:
	struct AcceptedSocket_t
	{
		AcceptedSocket_t(SocketHandle_t hSocket)
			: m_hSocket(hSocket)
			, m_Data(hSocket)
		{}

		SocketHandle_t            m_hSocket;
		netadr_t                  m_Address;
		CConnectedNetConsoleData  m_Data;
	};

private:
	CUtlVector<AcceptedSocket_t>  m_AcceptedSockets;
	SocketHandle_t                m_hListenSocket; // Used to accept connections.

	enum
	{
		SOCKET_TCP_MAX_ACCEPTS = 2
	};
};
