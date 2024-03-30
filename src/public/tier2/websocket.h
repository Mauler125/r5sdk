#ifndef TIER2_WEBSOCKETCREATOR_H
#define TIER2_WEBSOCKETCREATOR_H

#define WEBSOCKET_DEFAULT_BUFFER_SIZE 1024

struct ProtoWebSocketRefT;

class CWebSocket
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

	struct ConnParams_s
	{
		ConnParams_s()
		{
			bufSize = WEBSOCKET_DEFAULT_BUFFER_SIZE;
			retryTime = 0.0f;
			maxRetries = 0;

			timeOut = -1;
			keepAlive = -1;
			laxSSL = 0;
		}

		int32_t bufSize;
		float retryTime;
		int32_t maxRetries;

		int32_t timeOut;
		int32_t keepAlive;
		int32_t laxSSL;
	};

	struct ConnContext_s
	{
		ConnContext_s(const char* const addr)
		{
			webSocket = nullptr;
			address = addr;

			state = CS_CREATE;

			tryCount = 0;
			lastQueryTime = 0;
		}

		bool Connect(const double queryTime, const ConnParams_s& params);
		bool Status(const double queryTime);

		void SetParams(const ConnParams_s& params);

		void Disconnect();
		void Reconnect();
		void Destroy();

		ProtoWebSocketRefT* webSocket;
		ConnState_e state;

		int tryCount;
		double lastQueryTime;

		CUtlString address;
	};

	CWebSocket();

	bool Init(const char* const addressList, const ConnParams_s& params, const char*& initError);
	void Shutdown();

	bool SetupFromList(const char* const addressList);
	void UpdateParams(const ConnParams_s& params);

	void Update();
	void DeleteUnavailable();

	void DestroyAll();
	void ReconnectAll();
	void ClearAll();

	void SendData(const char* const dataBuf, const int32_t dataSize);
	bool IsInitialized() const;

private:
	bool m_initialized;
	ConnParams_s m_connParams;
	CUtlVector<ConnContext_s> m_addressList;
};

#endif // TIER2_WEBSOCKETCREATOR_H
