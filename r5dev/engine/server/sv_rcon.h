#pragma once
#include "tier1/NetAdr2.h"
#include "tier2/socketcreator.h"
#include "protoc/sv_rcon.pb.h"
#include "protoc/cl_rcon.pb.h"

constexpr char s_pszNoAuthMessage[]  = "This server is password protected for console access. Must send 'PASS <password>' command.\n\r";
constexpr char s_pszWrongPwMessage[] = "Password incorrect.\n\r";
constexpr char s_pszBannedMessage[]  = "Go away.\n\r";
constexpr char s_pszAuthMessage[]    = "RCON authentication succesfull.\n\r";

class CRConServer
{
public:
	~CRConServer() { delete m_pAdr2; delete m_pSocket; }

	void Init(void);
	void Shutdown(void);
	bool SetPassword(const char* pszPassword);

	void Think(void);
	void RunFrame(void);

	void Send(const std::string& svMessage) const;
	void Recv(void);

	std::string Serialize(const std::string& svRspBuf, const std::string& svRspVal, sv_rcon::response_t response_t) const;
	cl_rcon::request Deserialize(const std::string& svBuf) const;

	void Authenticate(const cl_rcon::request& cl_request, CConnectedNetConsoleData* pData);
	bool Comparator(std::string svPassword) const;

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
	std::string              m_svPasswordHash;
};
extern CRConServer* g_pRConServer;
CRConServer* RCONServer();