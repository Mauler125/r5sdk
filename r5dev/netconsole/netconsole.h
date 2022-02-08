//===========================================================================//
//
// Purpose: 
//
//===========================================================================//
#pragma once

constexpr const char* NETCON_VERSION = "2.0.0.1";

class CNetCon
{
public:
	bool Init(void);
	bool Shutdown(void);

	void TermSetup(void);
	void UserInput(void);

	void RunFrame(void);

	bool Connect(std::string svInAdr, std::string svInPort);

	void Send(std::string svMessage);
	void Recv(void);

	CNetAdr2* m_pNetAdr2 = new CNetAdr2("localhost", "37015");
	CSocketCreator* m_pSocket = new CSocketCreator();

	bool m_bInitialized      = false;
	bool m_bNoColor          = false;
	bool m_bQuitApplication  = false;
	std::atomic<bool> m_abPromptConnect{ true };
	std::atomic<bool> m_abConnEstablished{ false };
};