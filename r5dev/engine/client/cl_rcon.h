#pragma once
#include "tier1/NetAdr.h"
#include "tier2/socketcreator.h"
#include "protoc/sv_rcon.pb.h"
#include "protoc/cl_rcon.pb.h"
#include "engine/shared/base_rcon.h"

class CRConClient : public CNetConBase
{
public:
	CRConClient(void);
	~CRConClient(void);

	void Init(void);
	void Shutdown(void);

	void RunFrame(void);

	virtual void Disconnect(const char* szReason = nullptr) override;

	virtual bool ProcessMessage(const char* pMsgBuf, int nMsgLen) override;

	bool Serialize(vector<char>& vecBuf, const char* szReqBuf,
		const char* szReqVal, const cl_rcon::request_t requestType) const;

	bool IsInitialized(void) const;
	bool IsConnected(void);

	SocketHandle_t GetSocket(void);

private:
	bool m_bInitialized;
};

CRConClient* RCONClient();