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
// 
//-----------------------------------------------------------------------------
static void LiveAPI_ParamsChangedCallback(IConVar* var, const char* pOldValue)
{
	// TODO[ AMOS ]: latch this off to the server frame thread!
	LiveAPISystem()->UpdateParams();
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
static void LiveAPI_AddressChangedCallback(IConVar* var, const char* pOldValue)
{
	// TODO[ AMOS ]: latch this off to the server frame thread!
	LiveAPISystem()->InstallAddressList();
}

//-----------------------------------------------------------------------------
// console variables
//-----------------------------------------------------------------------------
ConVar liveapi_enabled("liveapi_enabled", "1", FCVAR_RELEASE | FCVAR_SERVER_FRAME_THREAD, "Enable LiveAPI functionality");
ConVar liveapi_session_name("liveapi_session_name", "liveapi_session", FCVAR_RELEASE | FCVAR_SERVER_FRAME_THREAD, "LiveAPI session name to identify this connection");

// WebSocket core
static ConVar liveapi_use_websocket("liveapi_use_websocket", "1", FCVAR_RELEASE | FCVAR_SERVER_FRAME_THREAD, "Use WebSocket to transmit LiveAPI events");
static ConVar liveapi_servers("liveapi_servers", "ws://127.0.0.1:7777", FCVAR_RELEASE | FCVAR_SERVER_FRAME_THREAD, "Comma separated list of addresses to connect to", &LiveAPI_AddressChangedCallback, "ws://domain.suffix:port");

// WebSocket connection base parameters
static ConVar liveapi_retry_count("liveapi_retry_count", "5", FCVAR_RELEASE | FCVAR_SERVER_FRAME_THREAD, "Amount of times to retry connecting before marking the connection as unavailable", &LiveAPI_ParamsChangedCallback);
static ConVar liveapi_retry_time("liveapi_retry_time", "30", FCVAR_RELEASE | FCVAR_SERVER_FRAME_THREAD, "Amount of time between each retry", &LiveAPI_ParamsChangedCallback);

// WebSocket connection context parameters
static ConVar liveapi_timeout("liveapi_timeout", "300", FCVAR_RELEASE | FCVAR_SERVER_FRAME_THREAD, "WebSocket connection timeout in seconds", &LiveAPI_ParamsChangedCallback);
static ConVar liveapi_keepalive("liveapi_keepalive", "30", FCVAR_RELEASE | FCVAR_SERVER_FRAME_THREAD, "Interval of time to send Pong to any connected server", &LiveAPI_ParamsChangedCallback);
static ConVar liveapi_lax_ssl("liveapi_lax_ssl", "1", FCVAR_RELEASE | FCVAR_SERVER_FRAME_THREAD, "Skip SSL certificate validation for all WSS connections (allows the use of self-signed certificates)", &LiveAPI_ParamsChangedCallback);

//-----------------------------------------------------------------------------
// constructors/destructors
//-----------------------------------------------------------------------------
LiveAPI::LiveAPI()
{
}

//-----------------------------------------------------------------------------
// Initialization of the LiveAPI system
//-----------------------------------------------------------------------------
void LiveAPI::Init()
{
	if (!liveapi_enabled.GetBool())
		return;

	if (liveapi_use_websocket.GetBool())
	{
		const char* initError = nullptr;

		if (!InitWebSocket(initError))
		{
			Error(eDLL_T::RTECH, 0, "LiveAPI: WebSocket initialization failed! [%s]\n", initError);
			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Shutdown of the LiveAPI system
//-----------------------------------------------------------------------------
void LiveAPI::Shutdown()
{
	webSocketSystem.Shutdown();
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void LiveAPI::CreateParams(CWebSocket::ConnParams_s& params)
{
	params.bufSize = LIVE_API_MAX_FRAME_BUFFER_SIZE;

	params.retryTime = liveapi_retry_time.GetFloat();
	params.maxRetries = liveapi_retry_count.GetInt();

	params.timeOut = liveapi_timeout.GetInt();
	params.keepAlive = liveapi_keepalive.GetInt();
	params.laxSSL = liveapi_lax_ssl.GetInt();
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void LiveAPI::UpdateParams()
{
	CWebSocket::ConnParams_s connParams;
	CreateParams(connParams);

	webSocketSystem.UpdateParams(connParams);
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
bool LiveAPI::InitWebSocket(const char*& initError)
{
	CWebSocket::ConnParams_s connParams;
	CreateParams(connParams);

	return webSocketSystem.Init(liveapi_servers.GetString(), connParams, initError);
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void LiveAPI::InstallAddressList()
{
	webSocketSystem.ClearAll();
	webSocketSystem.UpdateAddressList(liveapi_servers.GetString());
}

//-----------------------------------------------------------------------------
// LiveAPI state machine
//-----------------------------------------------------------------------------
void LiveAPI::RunFrame()
{
	if (!IsEnabled())
		return;

	if (liveapi_use_websocket.GetBool())
		webSocketSystem.Update();
}

//-----------------------------------------------------------------------------
// Send an event to all sockets
//-----------------------------------------------------------------------------
void LiveAPI::LogEvent(const char* const dataBuf, const int32_t dataSize)
{
	if (!IsEnabled())
		return;

	if (liveapi_use_websocket.GetBool())
		webSocketSystem.SendData(dataBuf, dataSize);
}

//-----------------------------------------------------------------------------
// Returns whether the system is enabled and able to run
//-----------------------------------------------------------------------------
bool LiveAPI::IsEnabled() const
{
	return liveapi_enabled.GetBool();
}

static LiveAPI s_liveApi;

//-----------------------------------------------------------------------------
// Singleton accessor
//-----------------------------------------------------------------------------
LiveAPI* LiveAPISystem()
{
	return &s_liveApi;
}
