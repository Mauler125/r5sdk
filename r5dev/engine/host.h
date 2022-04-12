#pragma once

inline CMemory p_Host_Error; /*48 89 4C 24 ? 48 89 54 24 ? 4C 89 44 24 ? 4C 89 4C 24 ? 53 57 48 81 EC ? ? ? ?*/
inline auto Host_Error = p_Host_Error.RCast<int(*)(char* error, ...)>();

inline bool* g_bAbortServerSet = nullptr;
inline jmp_buf* host_abortserver = nullptr;

///////////////////////////////////////////////////////////////////////////////
class HHost : public IDetour
{
	virtual void GetAdr(void) const
	{
		std::cout << "| FUN: Host_Error                           : 0x" << std::hex << std::uppercase << p_Host_Error.GetPtr() << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: g_bAbortServerSet                    : 0x" << std::hex << std::uppercase << g_bAbortServerSet     << std::setw(0)    << " |" << std::endl;
		std::cout << "| FUN: host_abortserver                     : 0x" << std::hex << std::uppercase << host_abortserver      << std::setw(0)    << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
	virtual void GetFun(void) const
	{
		p_Host_Error = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x4C\x24\x00\x48\x89\x54\x24\x00\x4C\x89\x44\x24\x00\x4C\x89\x4C\x24\x00\x53\x57\x48\x81\xEC\x00\x00\x00\x00"), "xxxx?xxxx?xxxx?xxxx?xxxxx????");
	}
	virtual void GetVar(void) const
	{
		g_bAbortServerSet = p_Host_Error.FindPattern("40 38 3D", CMemory::Direction::DOWN, 512, 4).ResolveRelativeAddress(3, 7).RCast<bool*>();
		host_abortserver = p_Host_Error.FindPattern("48 8D 0D", CMemory::Direction::DOWN, 512, 5).ResolveRelativeAddress(3, 7).RCast<jmp_buf*>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HHost);