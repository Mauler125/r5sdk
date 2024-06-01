#pragma once
#include "tier1/NetAdr.h"
#include "tier2/socketcreator.h"
#include "protoc/netcon.pb.h"
#include "engine/shared/base_rcon.h"

#define RCON_MIN_PASSWORD_LEN 8
#define RCON_MAX_BANNEDLIST_SIZE 512
#define RCON_SHA512_HASH_SIZE 64

class CRConServer : public CNetConBase
{
public:
	CRConServer(void);
	~CRConServer(void);

	void Init(const char* pPassword, const char* pNetKey = nullptr);
	void Shutdown(void);

	void Reboot(void);

	bool SetPassword(const char* pszPassword);
	bool SetWhiteListAddress(const char* pszAddress);

	void Think(void);
	void RunFrame(void);

	bool SendEncoded(const char* pResponseMsg, const char* pResponseVal,
		const netcon::response_e responseType,
		const int nMessageId = static_cast<int>(eDLL_T::NETCON),
		const int nMessageType = static_cast<int>(LogType_t::LOG_NET)) const;

	bool SendEncoded(const SocketHandle_t hSocket, const char* pResponseMsg,
		const char* pResponseVal, const netcon::response_e responseType,
		const int nMessageId = static_cast<int>(eDLL_T::NETCON),
		const int nMessageType = static_cast<int>(LogType_t::LOG_NET)) const;

	bool SendToAll(const char* pMsgBuf, const int nMsgLen) const;
	bool Serialize(vector<char>& vecBuf, const char* pResponseMsg, const char* pResponseVal, const netcon::response_e responseType,
		const int nMessageId = static_cast<int>(eDLL_T::NETCON), const int nMessageType = static_cast<int>(LogType_t::LOG_NET)) const;

	void Authenticate(const netcon::request& request, CConnectedNetConsoleData& data);
	bool Comparator(const string& svPassword) const;

	virtual bool ProcessMessage(const char* pMsgBuf, const int nMsgLen) override;

	void Execute(const netcon::request& request) const;
	bool CheckForBan(CConnectedNetConsoleData& data);

	virtual void Disconnect(const char* szReason = nullptr) override;
	void Disconnect(const int nIndex, const char* szReason = nullptr);
	void CloseNonAuthConnection(void);

	bool ShouldSend(const netcon::response_e responseType) const;
	bool IsInitialized(void) const;

	int GetAuthenticatedCount(void) const;
	void CloseAllSockets() { m_Socket.CloseAllAcceptedSockets(); }

private:
	int                      m_nConnIndex;
	int                      m_nAuthConnections;
	bool                     m_bInitialized;
	std::unordered_set<std::string> m_BannedList;
	uint8_t                  m_PasswordHash[RCON_SHA512_HASH_SIZE];
	netadr_t                 m_WhiteListAddress;
};

CRConServer* RCONServer();
