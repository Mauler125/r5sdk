#pragma once
#include "tier1/NetAdr2.h"
#include "tier2/socketcreator.h"
#include "protoc/sv_rcon.pb.h"
#include "protoc/cl_rcon.pb.h"

constexpr char s_pszNoAuthMessage[]  = "This server is password protected for console access. Must send 'PASS <password>' command.\n\r";
constexpr char s_pszWrongPwMessage[] = "Password incorrect.\n\r";
constexpr char s_pszBannedMessage[]  = "Go away.\n\r";

class CRConServer
{
public:
	void Init(void);
	void Think(void);

	void RunFrame(void);

	void Send(const std::string& svMessage) const;
	void Recv(void);

	std::string Serialize(const std::string& svRspBuf, const std::string& svRspVal, sv_rcon::response_t response_t) const;
	cl_rcon::request Deserialize(const std::string& svBuf) const;

	void Authenticate(const cl_rcon::request& cl_request, CConnectedNetConsoleData* pData);

	void ProcessBuffer(const char* pszIn, int nRecvLen, CConnectedNetConsoleData* pData);
	void ProcessMessage(const cl_rcon::request& cl_request);

	void Execute(const cl_rcon::request& cl_request) const;
	bool CheckForBan(CConnectedNetConsoleData* pData);

	void CloseConnection(void);
	void CloseNonAuthConnection(void);

private:

	bool                     m_bInitialized  = false;
	int                      m_nConnIndex    = 0;
	CNetAdr2*                m_pAdr2         = new CNetAdr2();
	CSocketCreator*          m_pSocket       = new CSocketCreator();
	std::vector<std::string> m_vBannedAddress;
};
extern CRConServer* g_pRConServer;