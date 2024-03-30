//===========================================================================//
// 
// Purpose: WebSocket implementation
// 
//===========================================================================//
#include "tier2/websocket.h"

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/proto/protossl.h"
#include "DirtySDK/proto/protowebsocket.h"


//-----------------------------------------------------------------------------
// constructors/destructors
//-----------------------------------------------------------------------------
CWebSocket::CWebSocket()
{
	m_initialized = false;
}

//-----------------------------------------------------------------------------
// Purpose: initialization of the socket system
//-----------------------------------------------------------------------------
bool CWebSocket::Init(const char* const addressList, const ConnParams_s& params, const char*& initError)
{
	if (!NetConnStatus('open', 0, NULL, 0))
	{
		initError = "Network connection module not initialized";
		return false;
	}

	if (!SetupFromList(addressList))
	{
		initError = (*addressList)
			? "Failed to parse address list"
			: "Address list is empty";

		return false;
	}

	m_connParams = params;
	m_initialized = true;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: shutdown of the socket system
//-----------------------------------------------------------------------------
void CWebSocket::Shutdown()
{
	m_initialized = false;
	ClearAll();
}

//-----------------------------------------------------------------------------
// Purpose: setup connection list, returns false if connection list is empty
//-----------------------------------------------------------------------------
bool CWebSocket::SetupFromList(const char* const addressList)
{
	const CUtlStringList addresses(addressList, ",");

	FOR_EACH_VEC(addresses, i)
	{
		const ConnContext_s conn(addresses[i]);
		m_addressList.AddToTail(conn);
	}

	return addresses.Count() != 0;
}

//-----------------------------------------------------------------------------
// Purpose: update parameters for each connection
//-----------------------------------------------------------------------------
void CWebSocket::UpdateParams(const ConnParams_s& params)
{
	m_connParams = params;

	for (ConnContext_s& conn : m_addressList)
	{
		if (conn.webSocket)
			conn.SetParams(params);
	}
}

//-----------------------------------------------------------------------------
// Purpose: socket state machine
//-----------------------------------------------------------------------------
void CWebSocket::Update()
{
	if (!IsInitialized())
		return;

	const double queryTime = Plat_FloatTime();

	for (ConnContext_s& conn : m_addressList)
	{
		if (conn.webSocket)
			ProtoWebSocketUpdate(conn.webSocket);

		if (conn.state == CS_CREATE || conn.state == CS_RETRY)
		{
			conn.Connect(queryTime, m_connParams);
			continue;
		}

		if (conn.state == CS_CONNECTED || conn.state == CS_LISTENING)
		{
			conn.Status(queryTime);
			continue;
		}

		if (conn.state == CS_DESTROYED)
		{
			if (conn.tryCount > m_connParams.maxRetries)
			{
				// All retry attempts have been used; mark unavailable for deletion
				conn.state = CS_UNAVAIL;
			}
			else
			{
				// Mark as retry, this will recreate the socket and reattempt
				// the connection
				conn.state = CS_RETRY;
			}
		}
	}

	DeleteUnavailable();
}

//-----------------------------------------------------------------------------
// Purpose: delete all connections marked unavailable
//-----------------------------------------------------------------------------
void CWebSocket::DeleteUnavailable()
{
	FOR_EACH_VEC_BACK(m_addressList, i)
	{
		if (m_addressList[i].state == CS_UNAVAIL)
		{
			m_addressList.FastRemove(i);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: destroy all connections
//-----------------------------------------------------------------------------
void CWebSocket::DestroyAll()
{
	for (ConnContext_s& conn : m_addressList)
	{
		conn.Destroy();
	}
}

//-----------------------------------------------------------------------------
// Purpose: reconnect all connections
//-----------------------------------------------------------------------------
void CWebSocket::ReconnectAll()
{
	for (ConnContext_s& conn : m_addressList)
	{
		conn.Reconnect();
	}
}

//-----------------------------------------------------------------------------
// Purpose: destroy and purge all connections
//-----------------------------------------------------------------------------
void CWebSocket::ClearAll()
{
	DestroyAll();
	m_addressList.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: send data to all sockets
//-----------------------------------------------------------------------------
void CWebSocket::SendData(const char* const dataBuf, const int32_t dataSize)
{
	if (!IsInitialized())
		return;

	for (ConnContext_s& conn : m_addressList)
	{
		if (conn.state != CS_LISTENING)
			continue;

		if (ProtoWebSocketSend(conn.webSocket, dataBuf, dataSize) < 0)
		{
			conn.Destroy(); // Reattempt the connection for this socket
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns whether the socket system is enabled and able to run
//-----------------------------------------------------------------------------
bool CWebSocket::IsInitialized() const
{
	return m_initialized;
}

//-----------------------------------------------------------------------------
// Purpose: connect to a socket
//-----------------------------------------------------------------------------
bool CWebSocket::ConnContext_s::Connect(const double queryTime, const ConnParams_s& params)
{
	const double retryTimeTotal = lastQueryTime + params.retryTime;
	const double currTime = Plat_FloatTime();

	if (retryTimeTotal > currTime)
		return false; // Still within retry period

	tryCount++;
	webSocket = ProtoWebSocketCreate(params.bufSize);

	if (!webSocket)
	{
		state = CS_UNAVAIL;
		return false;
	}

	SetParams(params);

	if (ProtoWebSocketConnect(webSocket, address.String()) != NULL)
	{
		// Failure
		Destroy();
		return false;
	}

	state = CS_CONNECTED;
	lastQueryTime = queryTime;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: check the connection status and destroy if not connected (-1)
//-----------------------------------------------------------------------------
bool CWebSocket::ConnContext_s::Status(const double queryTime)
{
	const int32_t status = ProtoWebSocketStatus(webSocket, 'stat', NULL, 0);

	if (status == -1)
	{
		Destroy();
		lastQueryTime = queryTime;

		return false;
	}
	else if (!status)
	{
		lastQueryTime = queryTime;
		return false;
	}

	tryCount = 0;
	state = CS_LISTENING;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: set parameters for this socket
//-----------------------------------------------------------------------------
void CWebSocket::ConnContext_s::SetParams(const ConnParams_s& params)
{
	Assert(webSocket);

	if (params.timeOut > 0)
	{
		ProtoWebSocketControl(webSocket, 'time', params.timeOut, 0, NULL);
	}

	if (params.keepAlive > 0)
	{
		ProtoWebSocketControl(webSocket, 'keep', params.keepAlive, 0, NULL);
	}

	ProtoWebSocketControl(webSocket, 'ncrt', params.laxSSL, 0, NULL);
	ProtoWebSocketUpdate(webSocket);
}

//-----------------------------------------------------------------------------
// Purpose: disconnect and mark socket as unavailable for removal
//-----------------------------------------------------------------------------
void CWebSocket::ConnContext_s::Disconnect()
{
	ProtoWebSocketDisconnect(webSocket);
	ProtoWebSocketUpdate(webSocket);
	ProtoWebSocketDestroy(webSocket);

	webSocket = nullptr;
	state = CS_UNAVAIL;
}

//-----------------------------------------------------------------------------
// Purpose: reconnect without burning retry attempts
//-----------------------------------------------------------------------------
void CWebSocket::ConnContext_s::Reconnect()
{
	Disconnect();
	state = CS_CREATE;
}

//-----------------------------------------------------------------------------
// Purpose: reconnect while burning retry attempts
//-----------------------------------------------------------------------------
void CWebSocket::ConnContext_s::Destroy()
{
	Disconnect();
	state = CS_DESTROYED;
}
