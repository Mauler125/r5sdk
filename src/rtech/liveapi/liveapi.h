#ifndef RTECH_LIVEAPI_H
#define RTECH_LIVEAPI_H
#include "tier2/websocket.h"
#include "thirdparty/protobuf/message.h"

#define LIVE_API_MAX_FRAME_BUFFER_SIZE 0x8000

extern ConVar liveapi_enabled;
extern ConVar liveapi_session_name;
extern ConVar liveapi_truncate_hash_fields;

struct ProtoWebSocketRefT;
typedef void (*LiveAPISendCallback_t)(ProtoWebSocketRefT* webSocket);

class LiveAPI
{
public:
	LiveAPI();
	~LiveAPI();

	void Init();
	void Shutdown();

	void ToggleInit();

	void CreateParams(CWebSocket::ConnParams_s& params);
	void UpdateParams();

	void InitWebSocket();
	void ShutdownWebSocket();

	void ToggleInitWebSocket();

	void RebootWebSocket();

	void CreateLogger();
	void DestroyLogger();

	void RunFrame();
	void LogEvent(const google::protobuf::Message* const toTransmit, const google::protobuf::Message* toPrint);

	bool IsEnabled() const;
	bool IsValidToRun() const;

	inline bool WebSocketInitialized() const { return webSocketSystem.IsInitialized(); }
	inline bool FileLoggerInitialized() const { return matchLogger != nullptr; }

private:
	CWebSocket webSocketSystem;

	std::shared_ptr<spdlog::logger> matchLogger;
	int matchLogCount;
	bool initialLog;
	bool initialized;
};

LiveAPI* LiveAPISystem();

#endif // RTECH_LIVEAPI_H
