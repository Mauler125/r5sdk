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
	CNetCon(void);
	~CNetCon(void);

	bool Init(void);
	bool Shutdown(void);

	void TermSetup(void);
	void UserInput(void);
	void ClearInput(void);

	void RunFrame(void);
	bool ShouldQuit(void) const;

	bool Connect(const std::string& svInAdr, const std::string& svInPort);
	void Disconnect(void);

	void Send(const std::string& svMessage) const;
	void Recv(void);

	void ProcessBuffer(const char* pRecvBuf, int nRecvLen, CConnectedNetConsoleData* pData);
	void ProcessMessage(const sv_rcon::response& sv_response) const;

	std::string Serialize(const std::string& svReqBuf, const std::string& svReqVal, const cl_rcon::request_t request_t) const;
	sv_rcon::response Deserialize(const std::string& svBuf) const;

private:
	bool m_bInitialized;
	bool m_bQuitApplication;
	bool m_bPromptConnect;
	bool m_bConnEstablished;
	float m_flTickInterval;

	CNetAdr m_Address;
	CSocketCreator m_Socket;
	std::string m_Input;

	mutable std::mutex m_Mutex;
};

//-----------------------------------------------------------------------------
// singleton
//-----------------------------------------------------------------------------
extern CNetCon* NetConsole();
