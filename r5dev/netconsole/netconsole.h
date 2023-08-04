//===========================================================================//
//
// Purpose: 
//
//===========================================================================//
#pragma once
#include "tier1/cmd.h"
#include "protoc/cl_rcon.pb.h"
#include "protoc/sv_rcon.pb.h"
#include "engine/shared/base_rcon.h"

constexpr const char* NETCON_VERSION = "2.0.0.1";

class CNetCon : public CNetConBase
{
public:
	CNetCon(void);
	~CNetCon(void);

	bool Init(const bool bAnsiColor);
	bool Shutdown(void);

	void TermSetup(const bool bAnsiColor);
	void UserInput(void);
	void ClearInput(void);

	void RunFrame(void);
	bool ShouldQuit(void) const;

	virtual void Disconnect(const char* szReason = nullptr);
	virtual bool ProcessMessage(const char* pMsgBuf, const int nMsgLen) override;

	bool Serialize(vector<char>& vecBuf, const char* szReqBuf,
		const char* szReqVal, const cl_rcon::request_t requestType) const;

	SocketHandle_t GetSocket(void);
	bool IsInitialized(void) const;
	bool IsConnected(void);

private:
	bool m_bInitialized;
	bool m_bQuitApplication;
	bool m_bPromptConnect;
	float m_flTickInterval;

	characterset_t m_CharacterSet;

	std::string m_Input;
	mutable std::mutex m_Mutex;
};

//-----------------------------------------------------------------------------
// singleton
//-----------------------------------------------------------------------------
extern CNetCon* NetConsole();
