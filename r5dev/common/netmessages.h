#pragma once
#include "tier1/bitbuf.h"

enum class UserMessages : int
{
	TextMsg = 0x2
};

class CNetMessage
{
public:
	void* iNetMessageVTable;
	int m_nGroup;
	bool m_bReliable;
	char padding[3];
	void* m_NetChannel;
};

class SVC_Print : public CNetMessage
{
public:
	bool Process();

	void* m_pMessageHandler;
	char padding[8];
	const char* m_szText;
private:
	char m_szTextBuffer[2048];
};

class SVC_UserMessage : public CNetMessage
{
public:

	bool Process();

	void* m_pMessageHandler;
	char padding[8];
	int			m_nMsgType;
	int			m_nLength;	// data length in bits
	bf_read		m_DataIn;
	bf_write	m_DataOut;
};

struct VecNetMessages
{
	CNetMessage** items;
	int64_t m_nAllocationCount;
	int64_t m_nGrowSize;
	int m_Size;
	int padding_;
};

struct VecNetDataFragments
{
	void** items;
	int64_t m_nAllocationCount;
	int64_t m_nGrowSize;
	int m_Size;
	int padding_;
};

//-------------------------------------------------------------------------
// MM_HEARTBEAT
//-------------------------------------------------------------------------
inline CMemory MM_Heartbeat__ToString; // server HeartBeat? (baseserver.cpp).

//-------------------------------------------------------------------------
// SVC_Print
//-------------------------------------------------------------------------
inline auto SVC_Print_Process = CMemory().RCast<bool(*)(SVC_Print* thisptr)>();
inline void* g_pSVC_Print_VTable;

//-------------------------------------------------------------------------
// SVC_UserMessage
//-------------------------------------------------------------------------
inline auto SVC_UserMessage_Process = CMemory().RCast<bool(*)(SVC_UserMessage* thisptr)>();
inline void* g_pSVC_UserMessage_VTable;

void CNetMessages_Attach();
void CNetMessages_Detach();

///////////////////////////////////////////////////////////////////////////////
class HMM_Heartbeat : public IDetour
{
	virtual void GetAdr(void) const
	{
		std::cout << "| FUN: MM_Heartbeat::ToString               : 0x" << std::hex << std::uppercase << MM_Heartbeat__ToString.GetPtr() << std::setw(nPad) << " |" << std::endl;
		std::cout << "| VAR: SVC_Print_VTable                     : 0x" << std::hex << std::uppercase << g_pSVC_Print_VTable             << std::setw(nPad) << " |" << std::endl;
		std::cout << "| VAR: SVC_UserMessage_VTable               : 0x" << std::hex << std::uppercase << g_pSVC_UserMessage_VTable       << std::setw(nPad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
	virtual void GetFun(void) const
	{
		MM_Heartbeat__ToString = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x83\xEC\x38\xE8\x00\x00\x00\x00\x3B\x05\x00\x00\x00\x00"), "xxxxx????xx????");
		// 0x1402312A0 // 48 83 EC 38 E8 ? ? ? ? 3B 05 ? ? ? ?
	}
	virtual void GetVar(void) const 
	{ 
		// We get the actual address of the vtable here, not the class instance.
		g_pSVC_Print_VTable = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x74\x1E\x48\x8D\x05\x00\x00\x00\x00\x89\x5F\x08"), "xxxxx????xxx").OffsetSelf(0x2).ResolveRelativeAddressSelf(0x3, 0x7);
		// 0x1402D21F6 74 1E 48 8D 05 ? ? ? ? 89 5F 08
		g_pSVC_UserMessage_VTable = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\xE8\x00\x00\x00\x00\x48\x85\xFF\x74\x65"), "x????xxxxx").OffsetSelf(0xD).ResolveRelativeAddressSelf(0x3, 0x7);
		// 0x1402D295E E8 ? ? ? ? 48 85 FF 74 65
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HMM_Heartbeat);
