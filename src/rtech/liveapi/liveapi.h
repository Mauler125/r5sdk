#ifndef RTECH_LIVEAPI_H
#define RTECH_LIVEAPI_H
#include "tier2/websocket.h"

#define LIVE_API_MAX_FRAME_BUFFER_SIZE 0x8000

extern ConVar liveapi_enabled;
extern ConVar liveapi_session_name;

struct ProtoWebSocketRefT;
typedef void (*LiveAPISendCallback_t)(ProtoWebSocketRefT* webSocket);

class LiveAPI
{
public:

	LiveAPI();

	void Init();
	void Shutdown();

	void CreateParams(CWebSocket::ConnParams_s& params);
	void UpdateParams();

	bool InitWebSocket(const char*& initError);
	void InstallAddressList();

	void RunFrame();
	void LogEvent(const char* const dataBuf, const int32_t dataSize);

	bool IsEnabled() const;
	inline bool WebSocketInitialized() const { return webSocketSystem.IsInitialized(); }

private:
	CWebSocket webSocketSystem;
};

LiveAPI* LiveAPISystem();

#endif // RTECH_LIVEAPI_H
