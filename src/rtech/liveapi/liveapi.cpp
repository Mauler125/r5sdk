//===========================================================================//
// 
// Purpose: LiveAPI WebSocket implementation
// 
//===========================================================================//
#include "liveapi.h"

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/proto/protossl.h"
#include "DirtySDK/proto/protowebsocket.h"

//-----------------------------------------------------------------------------
// console variables
//-----------------------------------------------------------------------------
static ConVar liveapi_enabled("liveapi_enabled", "1", FCVAR_RELEASE);
static ConVar liveapi_servers("liveapi_servers", "ws://127.0.0.1:7777" , FCVAR_RELEASE, "Comma separated list of addresses to connect to", "'ws://domain.suffix:port'");

static ConVar liveapi_timeout("liveapi_timeout", "300", FCVAR_RELEASE, "WebSocket connection timeout in seconds");
static ConVar liveapi_keepalive("liveapi_keepalive", "30", FCVAR_RELEASE, "Interval of time to send Pong to any connected server");
static ConVar liveapi_lax_ssl("liveapi_lax_ssl", "1", FCVAR_RELEASE, "Skip SSL certificate validation for all WSS connections (allows the use of self-signed certificates)");

static ConVar liveapi_retry_count("liveapi_retry_count", "5", FCVAR_RELEASE, "Amount of times to retry connecting before marking the connection as unavailable");
static ConVar liveapi_retry_time("liveapi_retry_time", "30", FCVAR_RELEASE, "Amount of time between each retry");

//-----------------------------------------------------------------------------
// constructors/destructors
//-----------------------------------------------------------------------------
LiveAPI::LiveAPI()
{
	initialized = false;
}

//-----------------------------------------------------------------------------
// Initialization of the LiveAPI system
//-----------------------------------------------------------------------------
void LiveAPI::Init()
{
	if (!liveapi_enabled.GetBool())
		return;

	NetConnStatus('open', 0, NULL, 0);

	const int32_t startupRet = NetConnStartup("-servicename=liveapi");

	if (startupRet < 0)
	{
		Error(eDLL_T::RTECH, 0, "LiveAPI: initialization failed! [%x]\n", startupRet);
		return;
	}

	ProtoSSLStartup();
	const vector<string> addresses = StringSplit(liveapi_servers.GetString(), ',');

	for (const string& addres : addresses)
	{
		const ConnContext_s conn(addres);
		servers.push_back(conn);
	}

	initialized = true;
}

//-----------------------------------------------------------------------------
// Shutdown of the LiveAPI system
//-----------------------------------------------------------------------------
void LiveAPI::Shutdown()
{
	initialized = false;

	for (ConnContext_s& conn : servers)
	{
		conn.Destroy();
	}

	servers.clear();
}

//-----------------------------------------------------------------------------
// LiveAPI state machine
//-----------------------------------------------------------------------------
void LiveAPI::RunFrame()
{
	if (!IsEnabled())
		return;

	const double queryTime = Plat_FloatTime();

	for (ConnContext_s& conn : servers)
	{
		if (conn.webSocket)
			ProtoWebSocketUpdate(conn.webSocket);

		if (conn.state == CS_CREATE || conn.state == CS_RETRY)
		{
			conn.Connect(queryTime);
			continue;
		}

		if (conn.state == CS_CONNECTED || conn.state == CS_LISTENING)
		{
			conn.Status(queryTime);
			continue;
		}

		if (conn.state == CS_DESTROYED)
		{
			if (conn.retryCount > liveapi_retry_count.GetInt())
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
// Delete all server connections marked unavailable
//-----------------------------------------------------------------------------
void LiveAPI::DeleteUnavailable()
{
	servers.erase(std::remove_if(servers.begin(), servers.end(),
		[](const ConnContext_s& conn)
		{
			return conn.state == CS_UNAVAIL;
		}
	), servers.end());
}

//-----------------------------------------------------------------------------
// Send an event to all sockets
//-----------------------------------------------------------------------------
void LiveAPI::SendEvent(const char* const dataBuf, const int32_t dataSize)
{
	for (ConnContext_s& conn : servers)
	{
		if (conn.state != CS_LISTENING)
			continue;

		if (ProtoWebSocketSend(conn.webSocket, dataBuf, dataSize) < 0)
			conn.Destroy(); // Reattempt the connection for this socket
	}
}

//-----------------------------------------------------------------------------
// Returns whether the system is enabled and able to run
//-----------------------------------------------------------------------------
bool LiveAPI::IsEnabled() const
{
	return initialized && liveapi_enabled.GetBool();
}

//-----------------------------------------------------------------------------
// Connect to a socket
//-----------------------------------------------------------------------------
bool LiveAPI::ConnContext_s::Connect(const double queryTime)
{
	const double retryTimeTotal = retryTime + liveapi_retry_time.GetFloat();
	const double currTime = Plat_FloatTime();

	if (retryTimeTotal > currTime)
		return false; // Still within retry period

	retryCount++;
	webSocket = ProtoWebSocketCreate(LIVE_API_MAX_FRAME_BUFFER_SIZE);

	if (!webSocket)
	{
		state = CS_UNAVAIL;
		return false;
	}

	const int32_t timeOut = liveapi_timeout.GetInt();

	if (timeOut > 0)
	{
		ProtoWebSocketControl(webSocket, 'time', timeOut, 0, NULL);
	}

	const int32_t keepAlive = liveapi_keepalive.GetInt();

	if (keepAlive > 0)
	{
		ProtoWebSocketControl(webSocket, 'keep', keepAlive, 0, NULL);
	}

	ProtoWebSocketControl(webSocket, 'ncrt', liveapi_lax_ssl.GetInt(), 0, NULL);
	ProtoWebSocketUpdate(webSocket);

	if (ProtoWebSocketConnect(webSocket, address.c_str()) != NULL)
	{
		// Failure
		Destroy();
		return false;
	}

	state = CS_CONNECTED;
	retryTime = queryTime;

	return true;
}

//-----------------------------------------------------------------------------
// Check the connection status and destroy if not connected (-1)
//-----------------------------------------------------------------------------
bool LiveAPI::ConnContext_s::Status(const double queryTime)
{
	const int32_t status = ProtoWebSocketStatus(webSocket, 'stat', NULL, 0);

	if (status == -1)
	{
		Destroy();
		retryTime = queryTime;

		return false;
	}
	else if (!status)
	{
		retryTime = queryTime;
		return false;
	}

	retryCount = 0;
	state = CS_LISTENING;

	return true;
}

//-----------------------------------------------------------------------------
// Destroy the connection
//-----------------------------------------------------------------------------
void LiveAPI::ConnContext_s::Destroy()
{
	ProtoWebSocketDisconnect(webSocket);
	ProtoWebSocketUpdate(webSocket);
	ProtoWebSocketDestroy(webSocket);

	webSocket = nullptr;
	state = CS_DESTROYED;
}

static LiveAPI s_liveApi;

//-----------------------------------------------------------------------------
// Singleton accessor
//-----------------------------------------------------------------------------
LiveAPI* LiveAPISystem()
{
	return &s_liveApi;
}
