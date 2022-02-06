#pragma once
#include "tier1/NetAdr2.h"

constexpr char s_pszNoAuthMessage[]  = "This server is password protected for console access. Must send 'PASS <password>' command.\n\r";
constexpr char s_pszWrongPwMessage[] = "Password incorrect.\n\r";
constexpr char s_pszBannedMessage[]  = "Go away.\n\r";

class CRConServer
{
public:
	void Init(void);
	void Think(void);

	void RunFrame(void);

	void ProcessMessage(void);
	void Authenticate(CConnectedNetConsoleData* pData);

	void HandleInputChars(const char* pIn, int recvLen, CConnectedNetConsoleData* pData);
	void Execute(CConnectedNetConsoleData* pData);

	bool CheckForBan(int nIdx, CConnectedNetConsoleData* pData);
	void CloseConnection(int nIdx);

private:

	bool                     m_bInitialized = false;
	CNetAdr2*                m_pAdr2        = new CNetAdr2();
	CSocketCreator*          m_pSocket      = new CSocketCreator();
	std::vector<std::string> m_vBannedAddress;
};
extern CRConServer* g_pRConServer;