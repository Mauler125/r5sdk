//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#pragma once
#include "tier1/bitbuf.h"
#include "public/inetmessage.h"
#include "public/inetmsghandler.h"

#define HUD_PRINTNOTIFY		1
#define HUD_PRINTCONSOLE	2
#define HUD_PRINTTALK		3
#define HUD_PRINTCENTER		4

//-------------------------------------------------------------------------------------
// Forward declarations
//-------------------------------------------------------------------------------------
class CNetChan;

class CNetMessage : public INetMessage
{
public:
	int m_nGroup;
	bool m_bReliable;
	CNetChan* m_NetChannel;
};

///////////////////////////////////////////////////////////////////////////////////////
// server messages:
///////////////////////////////////////////////////////////////////////////////////////
class SVC_Print : public CNetMessage, IServerMessageHandler
{
public:
	virtual	~SVC_Print() {};

	virtual void	SetNetChannel(INetChannel* netchan) = 0;
	virtual void	SetReliable(bool state) = 0;

	virtual bool	Process(void) = 0; bool ProcessImpl(void);

	virtual	bool	ReadFromBuffer(bf_read& buffer) = 0;
	virtual	bool	WriteToBuffer(bf_write& buffer) = 0;

	virtual bool	IsReliable(void) const = 0;

	virtual int          GetGroup(void) const = 0;
	virtual int          GetType(void) const = 0;
	virtual const char*  GetName(void) const = 0;
	virtual INetChannel* GetNetChannel(void) const = 0;
	virtual const char*  ToString(void) const = 0;
	virtual size_t       GetSize(void) const = 0;

	const void* m_pData;
	const char* m_szText;
private:
	char m_szTextBuffer[2048];
};

class SVC_UserMessage : public CNetMessage, IServerMessageHandler
{
public:
	virtual	~SVC_UserMessage() {};

	virtual void	SetNetChannel(INetChannel* netchan) = 0;
	virtual void	SetReliable(bool state) = 0;

	virtual bool	Process(void) = 0; bool ProcessImpl(void);

	virtual	bool	ReadFromBuffer(bf_read& buffer) = 0;
	virtual	bool	WriteToBuffer(bf_write& buffer) = 0;

	virtual bool	IsReliable(void) const = 0;

	virtual int          GetGroup(void) const = 0;
	virtual int          GetType(void) const = 0;
	virtual const char*  GetName(void) const = 0;
	virtual INetChannel* GetNetChannel(void) const = 0;
	virtual const char*  ToString(void) const = 0;
	virtual size_t       GetSize(void) const = 0;

	int			m_nMsgType;
	int			m_nLength;	// data length in bits
	bf_read		m_DataIn;
	bf_write	m_DataOut;
};

struct NET_StringCmd : CNetMessage, INetMessageHandler
{
	const char* cmd;
	char buffer[1024];
};


//-------------------------------------------------------------------------
// MM_HEARTBEAT
//-------------------------------------------------------------------------
inline CMemory MM_Heartbeat__ToString; // server HeartBeat? (baseserver.cpp).

//-------------------------------------------------------------------------
// SVC_Print
//-------------------------------------------------------------------------
inline auto SVC_Print_Process = CMemory().RCast<bool(*)(SVC_Print* thisptr)>();
inline void* g_pSVC_Print_VFTable = nullptr;

//-------------------------------------------------------------------------
// SVC_UserMessage
//-------------------------------------------------------------------------
inline auto SVC_UserMessage_Process = CMemory().RCast<bool(*)(SVC_UserMessage* thisptr)>();
inline void* g_pSVC_UserMessage_VFTable = nullptr;

void CNetMessages_Attach();
void CNetMessages_Detach();

///////////////////////////////////////////////////////////////////////////////
class HMM_Heartbeat : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: MM_Heartbeat::ToString               : {:#18x} |\n", MM_Heartbeat__ToString.GetPtr());
		spdlog::debug("| CON: SVC_Print                  (VFTable) : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_pSVC_Print_VFTable));
		spdlog::debug("| CON: SVC_UserMessage            (VFTable) : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_pSVC_UserMessage_VFTable));
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		MM_Heartbeat__ToString = g_GameDll.FindPatternSIMD("48 83 EC 38 E8 ?? ?? ?? ?? 3B 05 ?? ?? ?? ??");
		// 48 83 EC 38 E8 ? ? ? ? 3B 05 ? ? ? ?
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const
	{
		// We get the actual address of the vftable here, not the class instance.
		g_pSVC_Print_VFTable = g_GameDll.GetVirtualMethodTable(".?AVSVC_Print@@");
		g_pSVC_UserMessage_VFTable = g_GameDll.GetVirtualMethodTable(".?AVSVC_UserMessage@@");
	}
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HMM_Heartbeat);
