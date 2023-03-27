#pragma once
#include "tier1/NetAdr.h"
#include "tier2/socketcreator.h"
#include "protoc/sv_rcon.pb.h"
#include "protoc/cl_rcon.pb.h"

constexpr char s_pszNoAuthMessage[]  = "This server is password protected for console access; authenticate with 'PASS <password>' command.\n";
constexpr char s_pszWrongPwMessage[] = "Admin password incorrect.\n";
constexpr char s_pszBannedMessage[]  = "Go away.\n";
constexpr char s_pszAuthMessage[]    = "Authentication successful.\n";

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

	void Send(const std::string& responseMsg, const std::string& responseVal, const sv_rcon::response_t responseType,
		const int nMessageId = static_cast<int>(eDLL_T::NETCON), const int nMessageType = static_cast<int>(LogType_t::LOG_NET));
	void Send(const SocketHandle_t hSocket, const std::string& responseMsg, const std::string& responseVal,
		const sv_rcon::response_t responseType, const int nMessageId = static_cast<int>(eDLL_T::NETCON), const int nMessageType = static_cast<int>(LogType_t::LOG_NET));
	void Recv(void);

	std::string Serialize(const std::string& responseMsg, const std::string& responseVal, const sv_rcon::response_t responseType,
		const int nMessageId = static_cast<int>(eDLL_T::NETCON), const int nMessageType = static_cast<int>(LogType_t::LOG_NET)) const;
	cl_rcon::request Deserialize(const std::string& svBuf) const;

	void Authenticate(const cl_rcon::request& cl_request, CConnectedNetConsoleData* pData);
	bool Comparator(std::string svPassword) const;

	void ProcessBuffer(const char* pszIn, int nRecvLen, CConnectedNetConsoleData* pData);
	void ProcessMessage(const cl_rcon::request& cl_request);

	void Execute(const cl_rcon::request& cl_request, const bool bConVar) const;
	bool CheckForBan(CConnectedNetConsoleData* pData);

	void CloseConnection(void);
	void CloseNonAuthConnection(void);

	bool ShouldSend(const sv_rcon::response_t responseType) const;
	bool IsInitialized(void) const;

private:
	void Send(const std::string& svMessage) const;
	void Send(const SocketHandle_t hSocket, const std::string& svMessage) const;

	bool                     m_bInitialized;
	int                      m_nConnIndex;
	std::vector<std::string> m_BannedList;
	std::string              m_svPasswordHash;
	netadr_t                 m_Address;
	CSocketCreator           m_Socket;
};

CRConServer* RCONServer();