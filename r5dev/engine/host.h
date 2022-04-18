#pragma once

inline CMemory p_Host_RunFrame;
inline auto _Host_RunFrame = p_Host_RunFrame.RCast<void(*)(void* unused, float time)>();

inline CMemory p_Host_RunFrame_Render;
inline auto _Host_RunFrame_Render = p_Host_RunFrame_Render.RCast<void(*)(void)>();

inline CMemory p_Host_Error;
inline auto Host_Error = p_Host_Error.RCast<int(*)(char* error, ...)>();

inline CMemory p_VCR_EnterPausedState;
inline auto VCR_EnterPausedState = p_VCR_EnterPausedState.RCast<void(*)(void)>();

inline bool* g_bAbortServerSet = nullptr;
inline jmp_buf* host_abortserver = nullptr;

///////////////////////////////////////////////////////////////////////////////
class HHost : public IDetour
{
	virtual void GetAdr(void) const
	{
		std::cout << "| FUN: _Host_RunFrame                       : 0x" << std::hex << std::uppercase << p_Host_RunFrame.GetPtr()        << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: _Host_RunFrame_Render                : 0x" << std::hex << std::uppercase << p_Host_RunFrame_Render.GetPtr() << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: Host_Error                           : 0x" << std::hex << std::uppercase << p_Host_Error.GetPtr()           << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: VCR_EnterPausedState                 : 0x" << std::hex << std::uppercase << p_VCR_EnterPausedState.GetPtr() << std::setw(nPad) << " |" << std::endl;
		std::cout << "| VAR: host_abortserver                     : 0x" << std::hex << std::uppercase << host_abortserver                << std::setw(0)    << " |" << std::endl;
		std::cout << "| VAR: g_bAbortServerSet                    : 0x" << std::hex << std::uppercase << g_bAbortServerSet               << std::setw(0)    << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
	virtual void GetFun(void) const
	{
		p_Host_RunFrame = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\xC4\x48\x89\x58\x18\x48\x89\x70\x20\xF3\x0F\x11\x48\x00"), "xxxxxxxxxxxxxxx?");
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_Host_RunFrame_Render = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x57\x48\x83\xEC\x20\x48\x8B\x1D\x00\x00\x00\x00\x33\xFF"), "xxxx?xxxxxxxx????xx");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_Host_RunFrame_Render = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x20\x48\x8B\x0D\x00\x00\x00\x00\x48\x85\xC9\x75\x34"), "xxxxxxxxx????xxxxx");
#endif
		p_Host_Error = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x4C\x24\x00\x48\x89\x54\x24\x00\x4C\x89\x44\x24\x00\x4C\x89\x4C\x24\x00\x53\x57\x48\x81\xEC\x00\x00\x00\x00"), "xxxx?xxxx?xxxx?xxxx?xxxxx????");
		p_VCR_EnterPausedState = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x20\x65\x48\x8B\x04\x25\x00\x00\x00\x00\xBB\x00\x00\x00\x00\xC6\x05\x00\x00\x00\x00\x00"), "xxxxxxxxxxx????x????xx?????");

		_Host_RunFrame = p_Host_RunFrame.RCast<void(*)(void*, float)>();
		_Host_RunFrame_Render = p_Host_Error.RCast<void(*)(void)>();
		Host_Error = p_Host_Error.RCast<int(*)(char*, ...)>();
		VCR_EnterPausedState = p_VCR_EnterPausedState.RCast<void(*)(void)>();
	}
	virtual void GetVar(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		g_bAbortServerSet = p_Host_Error.FindPattern("40 38 3D", CMemory::Direction::DOWN, 512, 2).ResolveRelativeAddress(3, 7).RCast<bool*>();
		host_abortserver = p_Host_Error.FindPattern("48 8D 0D", CMemory::Direction::DOWN, 512, 3).ResolveRelativeAddress(3, 7).RCast<jmp_buf*>();
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		g_bAbortServerSet = p_Host_Error.FindPattern("40 38 3D", CMemory::Direction::DOWN, 512, 4).ResolveRelativeAddress(3, 7).RCast<bool*>();
		host_abortserver = p_Host_Error.FindPattern("48 8D 0D", CMemory::Direction::DOWN, 512, 5).ResolveRelativeAddress(3, 7).RCast<jmp_buf*>();
#endif
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HHost);