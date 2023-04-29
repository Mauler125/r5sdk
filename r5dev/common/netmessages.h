//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#pragma once
#include "tier1/bitbuf.h"
#include "public/inetchannel.h"
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
class SVC_Print;
class SVC_UserMessage;
class Base_CmdKeyValues;

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

//-------------------------------------------------------------------------
// SVC_ServerTick
//-------------------------------------------------------------------------
inline void* g_pSVC_ServerTick_VFTable = nullptr;

//-------------------------------------------------------------------------
// SVC_VoiceData
//-------------------------------------------------------------------------
inline void* g_pSVC_VoiceData_VFTable = nullptr;

//-------------------------------------------------------------------------
// Base_CmdKeyValues
//-------------------------------------------------------------------------
inline auto Base_CmdKeyValues_ReadFromBuffer = CMemory().RCast<bool(*)(Base_CmdKeyValues* thisptr, bf_read* buffer)>();
inline auto Base_CmdKeyValues_WriteToBuffer = CMemory().RCast<bool(*)(Base_CmdKeyValues* thisptr, bf_write* buffer)>();
inline void* g_pBase_CmdKeyValues_VFTable = nullptr;

//-------------------------------------------------------------------------
// Enumeration of the INetMessage class
//-------------------------------------------------------------------------
enum NetMessageVtbl
{
	Destructor = 0,
	SetNetChannel,
	SetReliable,
	Process,
	ReadFromBuffer,
	WriteToBuffer,
	IsReliable,
	GetGroup,
	GetType,
	GetName,
	GetNetChannel,
	ToString,
	GetSize
};

///////////////////////////////////////////////////////////////////////////////////////
class CNetMessage : public INetMessage
{
public:
	int m_nGroup;
	bool m_bReliable;
	CNetChan* m_NetChannel;
	INetMessageHandler* m_pMessageHandler;
};

///////////////////////////////////////////////////////////////////////////////////////
// server messages:
///////////////////////////////////////////////////////////////////////////////////////
class SVC_Print : public CNetMessage
{
public:
	virtual	~SVC_Print() {};

	virtual void	SetNetChannel(CNetChan* netchan) = 0;
	virtual void	SetReliable(bool state) = 0;

	virtual bool	Process(void) = 0; bool ProcessImpl(void);

	virtual	bool	ReadFromBuffer(bf_read* buffer) = 0;
	virtual	bool	WriteToBuffer(bf_write* buffer) = 0;

	virtual bool	IsReliable(void) const = 0;

	virtual int          GetGroup(void) const = 0;
	virtual int          GetType(void) const = 0;
	virtual const char*  GetName(void) const = 0;
	virtual CNetChan* GetNetChannel(void) const = 0;
	virtual const char*  ToString(void) const = 0;
	virtual size_t       GetSize(void) const = 0;

	const void* m_pData;
	const char* m_szText;
private:
	char m_szTextBuffer[2048];
};

class SVC_UserMessage : public CNetMessage
{
public:
	virtual	~SVC_UserMessage() {};

	virtual void	SetNetChannel(CNetChan* netchan) = 0;
	virtual void	SetReliable(bool state) = 0;

	virtual bool	Process(void) = 0; bool ProcessImpl(void);

	virtual	bool	ReadFromBuffer(bf_read* buffer) = 0;
	virtual	bool	WriteToBuffer(bf_write* buffer) = 0;

	virtual bool	IsReliable(void) const = 0;

	virtual int          GetGroup(void) const = 0;
	virtual int          GetType(void) const = 0;
	virtual const char*  GetName(void) const = 0;
	virtual CNetChan* GetNetChannel(void) const = 0;
	virtual const char*  ToString(void) const = 0;
	virtual size_t       GetSize(void) const = 0;

	int			m_nMsgType;
	int			m_nLength;	// data length in bits
	bf_read		m_DataIn;
	bf_write	m_DataOut;
};

class SVC_ServerTick : public CNetMessage
{
public:
	SVC_ServerTick() = default;
	SVC_ServerTick(int tick, float hostFrametime, float hostFrametime_stddeviation, uint8_t cpuPercent)
	{
		void** pVFTable = reinterpret_cast<void**>(this);
		*pVFTable = g_pSVC_ServerTick_VFTable;

		m_bReliable = false;
		m_NetChannel = nullptr;
		m_NetTick.m_nServerTick = tick;
		m_NetTick.m_nClientTick = tick;

		m_NetTick.m_flHostFrameTime = hostFrametime;
		m_NetTick.m_flHostFrameTimeStdDeviation = hostFrametime_stddeviation;
		m_NetTick.m_nServerCPU = cpuPercent;
	};

	virtual	~SVC_ServerTick() {};

	virtual void	SetNetChannel(CNetChan* netchan) { m_NetChannel = netchan; }
	virtual void	SetReliable(bool state) { m_bReliable = state; };

	virtual bool	Process(void)
	{
		return CallVFunc<bool>(NetMessageVtbl::Process, this);
	};
	virtual	bool	ReadFromBuffer(bf_read* buffer)
	{
		return CallVFunc<bool>(NetMessageVtbl::ReadFromBuffer, this, buffer);
	}
	virtual	bool	WriteToBuffer(bf_write* buffer)
	{
		return CallVFunc<bool>(NetMessageVtbl::WriteToBuffer, this, buffer);
	}

	virtual bool	IsReliable(void) const { return m_bReliable; };

	virtual int          GetGroup(void) const { return m_nGroup; };
	virtual int          GetType(void) const { return 22; };
	virtual const char* GetName(void) const { return "svc_ServerTick"; };
	virtual CNetChan* GetNetChannel(void) const { return m_NetChannel; };
	virtual const char* ToString(void) const
	{
		static char szBuf[4096];
		V_snprintf(szBuf, sizeof(szBuf), "%s: server tick %i", this->GetName(), m_NetTick.m_nServerTick);

		return szBuf;
	};
	virtual size_t       GetSize(void) const { return sizeof(SVC_ServerTick); };

	nettick_t m_NetTick;
};

struct SVC_VoiceData : public CNetMessage
{
public:
	SVC_VoiceData() = default;
	SVC_VoiceData(int senderClient, int nBytes, char* data)
	{
		void** pVFTable = reinterpret_cast<void**>(this);
		*pVFTable = g_pSVC_VoiceData_VFTable;

		m_bReliable = false;
		m_NetChannel = nullptr;

		m_nFromClient = senderClient;
		m_nLength = nBytes; // length in bits
		m_DataOut = data;

		m_nGroup = 2; // must be set to 2 to avoid being copied into replay buffer
	};

	virtual	~SVC_VoiceData() {};

	virtual void	SetNetChannel(CNetChan* netchan) { m_NetChannel = netchan; }
	virtual void	SetReliable(bool state) { m_bReliable = state; };

	virtual bool	Process(void)
	{
		return CallVFunc<bool>(NetMessageVtbl::Process, this);
	};
	virtual	bool	ReadFromBuffer(bf_read* buffer)
	{
		return CallVFunc<bool>(NetMessageVtbl::ReadFromBuffer, this, buffer);
	}
	virtual	bool	WriteToBuffer(bf_write* buffer)
	{
		return CallVFunc<bool>(NetMessageVtbl::WriteToBuffer, this, buffer);
	}

	virtual bool	IsReliable(void) const { return m_bReliable; };

	virtual int          GetGroup(void) const { return m_nGroup; };
	virtual int          GetType(void) const { return 14; };
	virtual const char* GetName(void) const { return "svc_VoiceData"; };
	virtual CNetChan* GetNetChannel(void) const { return m_NetChannel; };
	virtual const char* ToString(void) const
	{
		static char szBuf[4096];
		V_snprintf(szBuf, sizeof(szBuf), "%s: client %i, bytes %i", this->GetName(), m_nFromClient, ((m_nLength + 7) >> 3));

		return szBuf;
	};
	virtual size_t       GetSize(void) const { return sizeof(SVC_VoiceData); };

	int m_nFromClient;
	int m_nLength;
	bf_read m_DataIn;
	void* m_DataOut;
};


///////////////////////////////////////////////////////////////////////////////////////
// Client messages:
///////////////////////////////////////////////////////////////////////////////////////

struct NET_StringCmd : CNetMessage
{
	const char* cmd;
	char buffer[1024];
};

///////////////////////////////////////////////////////////////////////////////////////
// This message is subclassed by 'SVC_CmdKeyValues' and 'CLC_CmdKeyValues'
class Base_CmdKeyValues : public CNetMessage
{
public:
	virtual	~Base_CmdKeyValues() {};

	virtual void	SetNetChannel(CNetChan* netchan) = 0;
	virtual void	SetReliable(bool state) = 0;

	virtual bool	Process(void) = 0;

	virtual	bool	ReadFromBuffer(bf_read* buffer) = 0;
	static	bool	ReadFromBufferImpl(Base_CmdKeyValues* thisptr, bf_read* buffer);

	virtual	bool	WriteToBuffer(bf_write* buffer) = 0;
	static	bool	WriteToBufferImpl(Base_CmdKeyValues* thisptr, bf_write* buffer);

	virtual bool	IsReliable(void) const = 0;

	virtual int          GetGroup(void) const = 0;
	virtual int          GetType(void) const = 0;
	virtual const char* GetName(void) const = 0;
	virtual CNetChan* GetNetChannel(void) const = 0;
	virtual const char* ToString(void) const = 0;
	virtual size_t       GetSize(void) const = 0;

	int			m_nMsgType;
	int			m_nLength;	// data length in bits
	bf_read		m_DataIn;
	bf_write	m_DataOut;
};


///////////////////////////////////////////////////////////////////////////////
class V_NetMessages : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogConAdr("SVC_Print::`vftable'", reinterpret_cast<uintptr_t>(g_pSVC_Print_VFTable));
		LogConAdr("SVC_UserMessage::`vftable'", reinterpret_cast<uintptr_t>(g_pSVC_UserMessage_VFTable));
		LogConAdr("SVC_ServerTick::`vftable'", reinterpret_cast<uintptr_t>(g_pSVC_ServerTick_VFTable));
		LogConAdr("SVC_VoiceData::`vftable'", reinterpret_cast<uintptr_t>(g_pSVC_VoiceData_VFTable));
		LogConAdr("Base_CmdKeyValues::`vftable'", reinterpret_cast<uintptr_t>(g_pBase_CmdKeyValues_VFTable));
		LogFunAdr("MM_Heartbeat::ToString", MM_Heartbeat__ToString.GetPtr());
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
		g_pSVC_ServerTick_VFTable = g_GameDll.GetVirtualMethodTable(".?AVSVC_ServerTick@@");
		g_pSVC_VoiceData_VFTable = g_GameDll.GetVirtualMethodTable(".?AVSVC_VoiceData@@");
		g_pBase_CmdKeyValues_VFTable = g_GameDll.GetVirtualMethodTable(".?AVBase_CmdKeyValues@@");
	}
	virtual void Attach(void) const;
	virtual void Detach(void) const;
};
///////////////////////////////////////////////////////////////////////////////

