#pragma once

class CNetMessage
{
public:
	void* iNetMessageVTable;
	int m_nGroup;
	bool m_bReliable;
	char padding[3];
	void* m_NetChannel;
};

class SVC_Print : CNetMessage
{
public:
	void* m_pMessageHandler;
	char padding[8];
	const char* m_szText;

private:
	char m_szTextBuffer[2048];
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
inline CMemory p_SVC_Print_Process;
inline auto SVC_Print_Process = p_SVC_Print_Process.RCast<bool(*)(SVC_Print* thisptr)>();

inline void* g_pSVC_Print_VTable;

void CNetMessages_Attach();
void CNetMessages_Detach();

///////////////////////////////////////////////////////////////////////////////
class HMM_Heartbeat : public IDetour
{
	virtual void GetAdr(void) const
	{
		std::cout << "| FUN: MM_Heartbeat::ToString               : 0x" << std::hex << std::uppercase << MM_Heartbeat__ToString.GetPtr() << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: SVC_Print_Process                    : 0x" << std::hex << std::uppercase << p_SVC_Print_Process.GetPtr()    << std::setw(nPad) << " |" << std::endl;
		std::cout << "| VAR: SVC_Print_VTable                     : 0x" << std::hex << std::uppercase << g_pSVC_Print_VTable             << std::setw(nPad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
	virtual void GetFun(void) const
	{
		MM_Heartbeat__ToString = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x83\xEC\x38\xE8\x00\x00\x00\x00\x3B\x05\x00\x00\x00\x00"), "xxxxx????xx????");
		// 0x1402312A0 // 48 83 EC 38 E8 ? ? ? ? 3B 05 ? ? ? ?
		p_SVC_Print_Process = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\xD1\x48\x8B\x49\x18\x48\x8B\x01\x48\xFF\x60\x28"), "xxxxxxxxxxxxxx");
		// 0x1402D0810 // 48 8B D1 48 8B 49 18 48 8B 01 48 FF 60 28

		SVC_Print_Process = p_SVC_Print_Process.RCast<bool(*)(SVC_Print*)>();
	}
	virtual void GetVar(void) const 
	{ 
		// We get the actual address of the vtable here, not the class instance.
		g_pSVC_Print_VTable = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x74\x1E\x48\x8D\x05\x00\x00\x00\x00\x89\x5F\x08"), "xxxxx????xxx").OffsetSelf(0x2).ResolveRelativeAddressSelf(0x3, 0x7);
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HMM_Heartbeat);
