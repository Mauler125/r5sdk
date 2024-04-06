//===========================================================================//
// 
// Purpose: WebSocket implementation
// 
//===========================================================================//
#ifndef TIER2_WEBSOCKET_H
#define TIER2_WEBSOCKET_H

#define WEBSOCKET_DEFAULT_BUFFER_SIZE 1024

//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
struct ProtoWebSocketRefT;

class CWebSocket
{
public:
	enum ConnState_e
	{
		// The socket has to be created and setup
		CS_CREATE = 0,

		// The socket connection is established
		CS_CONNECTED,

		// The socket is listening for data
		CS_LISTENING,

		// The socket is destroyed and deallocated (if retries are set, the
		// code will set the state to 'CS_RETRY' and reattempt to establish
		// a connection up to ConnParams_s::maxRetries times
		CS_DESTROYED,

		// The socket was destroyed and deallocated, and marked for a retry
		// attempt
		CS_RETRY,

		// The socket was destroyed and deallocated, and is marked unavailable.
		// the code will remove this connection from the list and no further
		// attempts will be made
		CS_UNAVAIL
	};

	//-------------------------------------------------------------------------
	// Connection parameters for the system & each individual connection, if
	// these are changed, call CWebSocket::UpdateParams() to apply the new
	// parameters on the system and each connection
	//-------------------------------------------------------------------------
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

		// Total amount of buffer size that could be queued up and sent
		int32_t bufSize;

		// Total amount of time between each connection attempt
		float retryTime;

		// Maximum number of retries
		// NOTE: the initial attempt is not counted as a retry attempt; if this
		// field is set to 5, then the code will perform 1 connection attempt +
		// 5 retries before giving up and marking this connection as unavailable
		int32_t maxRetries;

		// Total amount of time in seconds before the connection is timing out
		int32_t timeOut;

		// Time interval in seconds for the periodical keepalive pong message
		int32_t keepAlive;

		// Whether to validate the clients certificate, if this is set, no
		// validation is performed
		int32_t laxSSL;
	};

	//-------------------------------------------------------------------------
	// Represents an individual socket connection
	//-------------------------------------------------------------------------
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
		bool Process(const double queryTime);

		void SetParams(const ConnParams_s& params);

		void Disconnect();
		void Reconnect();
		void Destroy();

		ProtoWebSocketRefT* webSocket;
		ConnState_e state;

		int tryCount; // Number of connection attempts
		double lastQueryTime;

		CUtlString address;
	};

	CWebSocket();

	bool Init(const char* const addressList, const ConnParams_s& params, const char*& initError);
	void Shutdown();

	bool UpdateAddressList(const char* const addressList);
	void UpdateParams(const ConnParams_s& params);

	void Update();
	void DeleteUnavailable();

	void DisconnectAll();
	void ReconnectAll();
	void ClearAll();

	void SendData(const char* const dataBuf, const int32_t dataSize);
	bool IsInitialized() const;

private:
	bool m_initialized;
	ConnParams_s m_connParams;
	CUtlVector<ConnContext_s> m_addressList;
};

#endif // TIER2_WEBSOCKET_H
