#pragma once
#include "tier1/NetAdr2.h"
#include "tier2/SocketCreator.h"
#include "protoc/sv_rcon.pb.h"
#include "protoc/cl_rcon.pb.h"

class CRConClient
{
public:
	CRConClient(void){};
	~CRConClient(void){};

	void Init(void);
	void Shutdown(void);

	void RunFrame(void);

	bool Connect(void);
	bool Connect(const std::string& svInAdr, const std::string& svInPort);
	void Disconnect(void);

	void Send(const std::string& svMessage) const;
	void Recv(void);

	void ProcessBuffer(const char* pszIn, int nRecvLen) const;
	void ProcessMessage(const sv_rcon::response& sv_response) const;

	std::string Serialize(const std::string& svReqBuf, const std::string& svReqVal, cl_rcon::request_t request_t) const;
	sv_rcon::response Deserialize(const std::string& svBuf) const;

	bool IsInitialized(void) const;
	bool IsConnected(void) const;

private:
	CNetAdr2* m_pNetAdr2 = new CNetAdr2("localhost", "37015");
	CSocketCreator* m_pSocket = new CSocketCreator();

	bool m_bInitialized = false;
	bool m_bConnEstablished = false;
};
extern CRConClient* g_pRConClient;