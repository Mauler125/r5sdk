//===========================================================================//
// 
// Purpose: LiveAPI WebSocket implementation
// 
//===========================================================================//
#include "liveapi.h"
#include "protobuf/util/json_util.h"

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/proto/protossl.h"
#include "DirtySDK/proto/protowebsocket.h"

//-----------------------------------------------------------------------------
// change callbacks
//-----------------------------------------------------------------------------
static void LiveAPI_EnabledChangedCallback(IConVar* var, const char* pOldValue)
{
	LiveAPISystem()->ToggleInit();
}
static void LiveAPI_WebSocketEnabledChangedCallback(IConVar* var, const char* pOldValue)
{
	LiveAPISystem()->ToggleInitWebSocket();
}
static void LiveAPI_ParamsChangedCallback(IConVar* var, const char* pOldValue)
{
	LiveAPISystem()->UpdateParams();
}
static void LiveAPI_AddressChangedCallback(IConVar* var, const char* pOldValue)
{
	LiveAPISystem()->RebootWebSocket();
}

//-----------------------------------------------------------------------------
// console variables
//-----------------------------------------------------------------------------
ConVar liveapi_enabled("liveapi_enabled", "0", FCVAR_RELEASE | FCVAR_SERVER_FRAME_THREAD, "Enable LiveAPI functionality", &LiveAPI_EnabledChangedCallback);
ConVar liveapi_session_name("liveapi_session_name", "liveapi_session", FCVAR_RELEASE | FCVAR_SERVER_FRAME_THREAD, "LiveAPI session name to identify this connection");
ConVar liveapi_truncate_hash_fields("liveapi_truncate_hash_fields", "1", FCVAR_RELEASE | FCVAR_SERVER_FRAME_THREAD, "Whether to truncate hash fields in LiveAPI events to save on I/O");

// WebSocket core
static ConVar liveapi_websocket_enabled("liveapi_websocket_enabled", "0", FCVAR_RELEASE | FCVAR_SERVER_FRAME_THREAD, "Whether to use WebSocket to transmit LiveAPI events", &LiveAPI_WebSocketEnabledChangedCallback);
static ConVar liveapi_servers("liveapi_servers", "", FCVAR_RELEASE | FCVAR_SERVER_FRAME_THREAD, "Comma separated list of addresses to connect to", &LiveAPI_AddressChangedCallback, "ws://domain.suffix:port");

// WebSocket connection base parameters
static ConVar liveapi_retry_count("liveapi_retry_count", "5", FCVAR_RELEASE | FCVAR_SERVER_FRAME_THREAD, "Amount of times to retry connecting before marking the connection as unavailable", &LiveAPI_ParamsChangedCallback);
static ConVar liveapi_retry_time("liveapi_retry_time", "30", FCVAR_RELEASE | FCVAR_SERVER_FRAME_THREAD, "Amount of time between each retry", &LiveAPI_ParamsChangedCallback);

// WebSocket connection context parameters
static ConVar liveapi_timeout("liveapi_timeout", "300", FCVAR_RELEASE | FCVAR_SERVER_FRAME_THREAD, "WebSocket connection timeout in seconds", &LiveAPI_ParamsChangedCallback);
static ConVar liveapi_keepalive("liveapi_keepalive", "30", FCVAR_RELEASE | FCVAR_SERVER_FRAME_THREAD, "Interval of time to send Pong to any connected server", &LiveAPI_ParamsChangedCallback);
static ConVar liveapi_lax_ssl("liveapi_lax_ssl", "1", FCVAR_RELEASE | FCVAR_SERVER_FRAME_THREAD, "Skip SSL certificate validation for all WSS connections (allows the use of self-signed certificates)", &LiveAPI_ParamsChangedCallback);

// Print core
static ConVar liveapi_print_enabled("liveapi_print_enabled", "0", FCVAR_RELEASE | FCVAR_SERVER_FRAME_THREAD, "Whether to enable the printing of all events to a LiveAPI JSON file");

// Print parameters
static ConVar liveapi_print_pretty("liveapi_print_pretty", "0", FCVAR_RELEASE | FCVAR_SERVER_FRAME_THREAD, "Whether to print events in a formatted manner to the LiveAPI JSON file");
static ConVar liveapi_print_primitive("liveapi_print_primitive", "0", FCVAR_RELEASE | FCVAR_SERVER_FRAME_THREAD, "Whether to print primitive event fields to the LiveAPI JSON file");

//-----------------------------------------------------------------------------
// constructors/destructors
//-----------------------------------------------------------------------------
LiveAPI::LiveAPI()
{
	matchLogCount = 0;
	initialLog = false;
	initialized = false;
}
LiveAPI::~LiveAPI()
{
}

//-----------------------------------------------------------------------------
// Initialization of the LiveAPI system
//-----------------------------------------------------------------------------
void LiveAPI::Init()
{
	if (!liveapi_enabled.GetBool())
		return;

	InitWebSocket();
	initialized = true;
}

//-----------------------------------------------------------------------------
// Shutdown of the LiveAPI system
//-----------------------------------------------------------------------------
void LiveAPI::Shutdown()
{
	webSocketSystem.Shutdown();
	DestroyLogger();
	initialized = false;
}

//-----------------------------------------------------------------------------
// Toggle between init or deinit depending on current init state and the value
// of the cvar 'liveapi_enabled'
//-----------------------------------------------------------------------------
void LiveAPI::ToggleInit()
{
	const bool enabled = liveapi_enabled.GetBool();

	if (enabled && !initialized)
		Init();
	else if (!enabled && initialized)
		Shutdown();
}

//-----------------------------------------------------------------------------
// Populate the connection params structure
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
// Update the websocket parameters and apply them on all connections
//-----------------------------------------------------------------------------
void LiveAPI::UpdateParams()
{
	CWebSocket::ConnParams_s connParams;
	CreateParams(connParams);

	webSocketSystem.UpdateParams(connParams);
}

//-----------------------------------------------------------------------------
// Initialize the websocket system
//-----------------------------------------------------------------------------
void LiveAPI::InitWebSocket()
{
	if (!liveapi_websocket_enabled.GetBool())
		return;

	CWebSocket::ConnParams_s connParams;
	CreateParams(connParams);

	const char* initError = nullptr;

	if (!webSocketSystem.Init(liveapi_servers.GetString(), connParams, initError))
	{
		Error(eDLL_T::RTECH, 0, "LiveAPI: WebSocket initialization failed! [%s]\n", initError);
		return;
	}
}

//-----------------------------------------------------------------------------
// Shutdown the websocket system
//-----------------------------------------------------------------------------
void LiveAPI::ShutdownWebSocket()
{
	webSocketSystem.Shutdown();
}

//-----------------------------------------------------------------------------
// Toggle between init or deinit depending on current init state and the value
// of the cvar 'liveapi_websocket_enabled'
//-----------------------------------------------------------------------------
void LiveAPI::ToggleInitWebSocket()
{
	const bool enabled = liveapi_websocket_enabled.GetBool();

	if (enabled && !WebSocketInitialized())
		InitWebSocket();
	else if (!enabled && WebSocketInitialized())
		ShutdownWebSocket();
}

//-----------------------------------------------------------------------------
// Reboot the websocket system and reconnect to addresses specified in cvar
// 'liveapi_servers'
//-----------------------------------------------------------------------------
void LiveAPI::RebootWebSocket()
{
	ShutdownWebSocket();
	InitWebSocket();
}

//-----------------------------------------------------------------------------
// Create the file logger
//-----------------------------------------------------------------------------
void LiveAPI::CreateLogger()
{
	// Its possible that one was already created but never closed, this is
	// possible if the game scripts crashed or something along those lines.
	DestroyLogger();

	if (!liveapi_print_enabled.GetBool())
		return; // Logging is disabled

	matchLogger = spdlog::basic_logger_mt("match_logger",
		Format("platform/liveapi/logs/%s/match_%d.json", g_LogSessionUUID.c_str(), matchLogCount++));

	matchLogger.get()->set_pattern("%v");
	matchLogger.get()->info("[\n");
}

//-----------------------------------------------------------------------------
// Destroy the file logger
//-----------------------------------------------------------------------------
void LiveAPI::DestroyLogger()
{
	if (initialLog)
		initialLog = false;

	if (!matchLogger)
		return; // Nothing to drop

	matchLogger.get()->info("\n]\n");
	matchLogger.reset();

	spdlog::drop("match_logger");
}

//-----------------------------------------------------------------------------
// LiveAPI state machine
//-----------------------------------------------------------------------------
void LiveAPI::RunFrame()
{
	if (!IsEnabled())
		return;

	if (WebSocketInitialized())
		webSocketSystem.Update();
}

//-----------------------------------------------------------------------------
// Send an event to all sockets
//-----------------------------------------------------------------------------
void LiveAPI::LogEvent(const google::protobuf::Message* const toTransmit, const google::protobuf::Message* toPrint)
{
	if (!IsEnabled())
		return;

	if (WebSocketInitialized())
	{
		const string data = toTransmit->SerializeAsString();
		webSocketSystem.SendData(data.c_str(), (int)data.size());
	}

	// NOTE: we don't check on the cvar 'liveapi_print_enabled' here because if
	// this cvar gets disabled on the fly and we check it here, the output will
	// be truncated and thus invalid! Log for as long as the SpdLog instance is
	// valid.
	if (matchLogger)
	{
		std::string jsonStr(initialLog ? ",\n" : "");
		google::protobuf::util::JsonPrintOptions options;

		options.add_whitespace = liveapi_print_pretty.GetBool();
		options.always_print_primitive_fields = liveapi_print_primitive.GetBool();

		google::protobuf::util::MessageToJsonString(*toPrint, &jsonStr, options);

		// Remove the trailing newline character
		if (options.add_whitespace && !jsonStr.empty())
			jsonStr.pop_back();

		matchLogger.get()->info(jsonStr);

		if (!initialLog)
			initialLog = true;
	}
}

//-----------------------------------------------------------------------------
// Returns whether the system is enabled
//-----------------------------------------------------------------------------
bool LiveAPI::IsEnabled() const
{
	return liveapi_enabled.GetBool();
}

//-----------------------------------------------------------------------------
// Returns whether the system is able to run
//-----------------------------------------------------------------------------
bool LiveAPI::IsValidToRun() const
{
	return (IsEnabled() && (WebSocketInitialized() || FileLoggerInitialized()));
}

static LiveAPI s_liveApi;

//-----------------------------------------------------------------------------
// Singleton accessor
//-----------------------------------------------------------------------------
LiveAPI* LiveAPISystem()
{
	return &s_liveApi;
}
