//===========================================================================//
//
// Purpose: 
//
//===========================================================================//
#pragma once
#include "protoc/cl_rcon.pb.h"
#include "protoc/sv_rcon.pb.h"

constexpr const char* NETCON_VERSION = "2.0.0.1";

class CNetCon
{
public:
	~CNetCon() { delete m_pNetAdr2; delete m_pSocket; }

	bool Init(void);
	bool Shutdown(void);

	void TermSetup(void);
	void UserInput(void);

	void RunFrame(void);
	bool ShouldQuit(void) const;

	bool Connect(const std::string& svInAdr, const std::string& svInPort);
	void Disconnect(void);

	void Send(const std::string& svMessage) const;
	void Recv(void);

	void ProcessBuffer(const char* pszIn, int nRecvLen) const;
	void ProcessMessage(const sv_rcon::response& sv_response) const;

	std::string Serialize(const std::string& svReqBuf, const std::string& svReqVal, cl_rcon::request_t request_t) const;
	sv_rcon::response Deserialize(const std::string& svBuf) const;

private:
	CNetAdr2* m_pNetAdr2 = new CNetAdr2("localhost", "37015");
	CSocketCreator* m_pSocket = new CSocketCreator();

	bool m_bInitialized      = false;
	bool m_bNoColor          = false;
	bool m_bQuitApplication  = false;
	std::atomic<bool> m_abPromptConnect{ true };
	std::atomic<bool> m_abConnEstablished{ false };
};