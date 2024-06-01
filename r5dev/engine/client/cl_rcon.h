#pragma once
#include "tier1/NetAdr.h"
#include "tier2/socketcreator.h"
#include "protoc/netcon.pb.h"
#include "engine/shared/base_rcon.h"

class CRConClient : public CNetConBase
{
public:
	CRConClient(void);
	~CRConClient(void);

	void Init(const char* pNetKey = nullptr);
	void Shutdown(void);
	void RunFrame(void);

	virtual void Disconnect(const char* szReason = nullptr) override;
	virtual bool ProcessMessage(const char* pMsgBuf, const int nMsgLen) override;

	bool Serialize(vector<char>& vecBuf, const char* szReqBuf,
		const char* szReqVal, const netcon::request_e requestType) const;

	void RequestConsoleLog(const bool bWantLog);
	bool ShouldReceive(void);

	bool IsRemoteLocal(void);
	bool IsInitialized(void) const;
	bool IsConnected(void);

	CConnectedNetConsoleData* GetData(void);
	SocketHandle_t GetSocket(void);

private:
	bool m_bInitialized;
};

CRConClient* RCONClient();