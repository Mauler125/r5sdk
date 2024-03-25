#ifndef RTECH_LIVEAPI_H
#define RTECH_LIVEAPI_H

#define LIVE_API_MAX_FRAME_BUFFER_SIZE 0x8000

struct ProtoWebSocketRefT;
typedef void (*LiveAPISendCallback_t)(ProtoWebSocketRefT* webSocket);

class LiveAPI
{
public:
	enum ConnState_e
	{
		CS_CREATE = 0,

		CS_CONNECTED,
		CS_LISTENING,

		CS_DESTROYED,

		CS_RETRY,
		CS_UNAVAIL
	};

	struct ConnContext_s
	{
		ConnContext_s(const string& addr)
		{
			webSocket = nullptr;
			address = addr;

			state = CS_CREATE;

			retryCount = 0;
			retryTime = 0;
		}

		bool Connect(const double queryTime);
		bool Process(const double queryTime);

		void Destroy();

		ProtoWebSocketRefT* webSocket;
		ConnState_e state;

		int retryCount;
		double retryTime;

		string address;
	};

	LiveAPI();

	void Init();
	void Shutdown();

	void RunFrame();
	void DeleteUnavailable();

	void SendEvent(const char* const dataBuf, const int32_t dataSize);
	bool IsEnabled() const;

private:
	bool initialized;
	vector<ConnContext_s> servers;
};

LiveAPI* LiveAPISystem();

#endif // RTECH_LIVEAPI_H
