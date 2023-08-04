#pragma once
#include "tier1/NetAdr.h"
#include "tier2/socketcreator.h"
#include "protoc/sv_rcon.pb.h"
#include "protoc/cl_rcon.pb.h"
#include "engine/shared/base_rcon.h"

#define RCON_MIN_PASSWORD_LEN 8
#define RCON_MAX_BANNEDLIST_SIZE 512

class CRConServer : public CNetConBase
{
public:
	CRConServer(void);
	~CRConServer(void);

	void Init(void);
	void Shutdown(void);

	bool SetPassword(const char* pszPassword);
	bool SetWhiteListAddress(const char* pszAddress);

	void Think(void);
	void RunFrame(void);

	bool SendEncode(const char* pResponseMsg, const char* pResponseVal,
		const sv_rcon::response_t responseType,
		const int nMessageId = static_cast<int>(eDLL_T::NETCON),
		const int nMessageType = static_cast<int>(LogType_t::LOG_NET)) const;

	bool SendEncode(const SocketHandle_t hSocket, const char* pResponseMsg,
		const char* pResponseVal, const sv_rcon::response_t responseType,
		const int nMessageId = static_cast<int>(eDLL_T::NETCON),
		const int nMessageType = static_cast<int>(LogType_t::LOG_NET)) const;

	bool SendToAll(const char* pMsgBuf, const int nMsgLen) const;
	bool Serialize(vector<char>& vecBuf, const char* pResponseMsg, const char* pResponseVal, const sv_rcon::response_t responseType,
		const int nMessageId = static_cast<int>(eDLL_T::NETCON), const int nMessageType = static_cast<int>(LogType_t::LOG_NET)) const;

	void Authenticate(const cl_rcon::request& request, CConnectedNetConsoleData& data);
	bool Comparator(const string& svPassword) const;

	virtual bool ProcessMessage(const char* pMsgBuf, const int nMsgLen) override;

	void Execute(const cl_rcon::request& request) const;
	bool CheckForBan(CConnectedNetConsoleData& data);

	virtual void Disconnect(const char* szReason = nullptr) override;
	void Disconnect(const int nIndex, const char* szReason = nullptr);
	void CloseNonAuthConnection(void);

	bool ShouldSend(const sv_rcon::response_t responseType) const;
	bool IsInitialized(void) const;

	int GetAuthenticatedCount(void) const;

private:
	int                      m_nConnIndex;
	int                      m_nAuthConnections;
	bool                     m_bInitialized;
	std::unordered_set<std::string> m_BannedList;
	std::string              m_svPasswordHash;
	netadr_t                 m_WhiteListAddress;
};

CRConServer* RCONServer();
