#pragma once
#include "tier1/bitbuf.h"
#include "public/inetmsghandler.h"

enum class UserMessages : int
{
	TextMsg = 0x2
};

class INetMessage
{
	void* __vftable /*VFT*/;
};

class CNetMessage : public INetMessage
{
public:
	int m_nGroup;
	bool m_bReliable;
	char padding[3];
	void* m_NetChannel;
};

class SVC_Print : public CNetMessage, IServerMessageHandler
{
public:
	bool Process();

	char padding[8];
	const char* m_szText;
private:
	char m_szTextBuffer[2048];
};

class SVC_UserMessage : public CNetMessage, IServerMessageHandler
{
public:
	bool Process();

	int			m_nMsgType;
	int			m_nLength;	// data length in bits
	bf_read		m_DataIn;
	bf_write	m_DataOut;
};

//-------------------------------------------------------------------------
// MM_HEARTBEAT
//-------------------------------------------------------------------------
inline CMemory MM_Heartbeat__ToString; // server HeartBeat? (baseserver.cpp).

//-------------------------------------------------------------------------
// SVC_Print
//-------------------------------------------------------------------------
inline auto SVC_Print_Process = CMemory().RCast<bool(*)(SVC_Print* thisptr)>();
inline void* g_pSVC_Print_VTable = nullptr;

//-------------------------------------------------------------------------
// SVC_UserMessage
//-------------------------------------------------------------------------
inline auto SVC_UserMessage_Process = CMemory().RCast<bool(*)(SVC_UserMessage* thisptr)>();
inline void* g_pSVC_UserMessage_VTable = nullptr;

void CNetMessages_Attach();
void CNetMessages_Detach();

///////////////////////////////////////////////////////////////////////////////
class HMM_Heartbeat : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: MM_Heartbeat::ToString               : {:#18x} |\n", MM_Heartbeat__ToString.GetPtr());
		spdlog::debug("| VAR: SVC_Print_VTable                     : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_pSVC_Print_VTable));
		spdlog::debug("| VAR: SVC_UserMessage_VTable               : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_pSVC_UserMessage_VTable));
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		MM_Heartbeat__ToString = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x83\xEC\x38\xE8\x00\x00\x00\x00\x3B\x05\x00\x00\x00\x00"), "xxxxx????xx????");
		// 48 83 EC 38 E8 ? ? ? ? 3B 05 ? ? ? ?
	}
	virtual void GetVar(void) const 
	{ 
		// We get the actual address of the vtable here, not the class instance.
		g_pSVC_Print_VTable = g_GameDll.GetVirtualMethodTable(".?AVSVC_Print@@");
		g_pSVC_UserMessage_VTable = g_GameDll.GetVirtualMethodTable(".?AVSVC_UserMessage@@");
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HMM_Heartbeat);
