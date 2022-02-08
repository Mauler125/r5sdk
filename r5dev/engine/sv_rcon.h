#pragma once
#include "tier1/NetAdr2.h"
#include "tier2/socketcreator.h"
#include "common/igameserverdata.h"

constexpr char s_pszNoAuthMessage[]  = "This server is password protected for console access. Must send 'PASS <password>' command.\n\r";
constexpr char s_pszWrongPwMessage[] = "Password incorrect.\n\r";
constexpr char s_pszBannedMessage[]  = "Go away.\n\r";

class CRConServer
{
public:
	void Init(void);
	void Think(void);

	void RunFrame(void);

	void Recv(void);
	void Send(const char* pszBuf);
	void Auth(CConnectedNetConsoleData* pData);

	void HandleInputChars(const char* pszIn, int nRecvLen, CConnectedNetConsoleData* pData);
	void Execute(CConnectedNetConsoleData* pData);

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