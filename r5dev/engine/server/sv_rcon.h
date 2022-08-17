#pragma once
#include "tier1/NetAdr2.h"
#include "tier2/socketcreator.h"
#include "protoc/sv_rcon.pb.h"
#include "protoc/cl_rcon.pb.h"

constexpr char s_pszNoAuthMessage[]  = "This server is password protected for console access. Authenticate with 'PASS <password>' command.\n";
constexpr char s_pszWrongPwMessage[] = "Admin password incorrect.\n";
constexpr char s_pszBannedMessage[]  = "Go away.\n";
constexpr char s_pszAuthMessage[]    = "Authentication successfull.\n";

class CRConServer
{
public:
	CRConServer(void);
	~CRConServer(void);

	void Init(void);
	void Shutdown(void);
	bool SetPassword(const char* pszPassword);

	void Think(void);
	void RunFrame(void);

	void Send(const std::string& svMessage) const;
	void Send(SocketHandle_t hSocket, const std::string& svMessage) const;
	void Send(const std::string& svRspBuf, const std::string& svRspVal, sv_rcon::response_t responseType, int nResponseId = -4);
	void Send(SocketHandle_t hSocket, const std::string& svRspBuf, const std::string& svRspVal, sv_rcon::response_t responseType, int nResponseId = -4);
	void Recv(void);

	std::string Serialize(const std::string& svRspBuf, const std::string& svRspVal, sv_rcon::response_t responseType, int nResponseId = -4) const;
	cl_rcon::request Deserialize(const std::string& svBuf) const;

	void Authenticate(const cl_rcon::request& cl_request, CConnectedNetConsoleData* pData);
	bool Comparator(std::string svPassword) const;

	void ProcessBuffer(const char* pszIn, int nRecvLen, CConnectedNetConsoleData* pData);
	void ProcessMessage(const cl_rcon::request& cl_request);

	void Execute(const cl_rcon::request& cl_request, bool bConVar) const;
	bool CheckForBan(CConnectedNetConsoleData* pData);

	void CloseConnection(void);
	void CloseNonAuthConnection(void);

	bool IsInitialized(void) const;

private:

	bool                     m_bInitialized;
	int                      m_nConnIndex;
	CNetAdr2*                m_pAdr2;
	CSocketCreator*          m_pSocket;
	std::vector<std::string> m_vBannedAddress;
	std::string              m_svPasswordHash;
};
extern CRConServer* g_pRConServer;
CRConServer* RCONServer();