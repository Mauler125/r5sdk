#pragma once
#include "engine/debugoverlay.h"

inline bool* cl_m_bPaused = nullptr;
inline int* cl_host_tickcount = nullptr;

///////////////////////////////////////////////////////////////////////////////
class CBaseClientState
{
public:
	bool IsPaused();
	float GetClientTime();
	int GetClientTickCount() const;	// Get the client tick count.
	void SetClientTickCount(int tick); // Set the client tick count.
};
extern CBaseClientState* g_pBaseClientState;

/* ==== CCLIENTSTATE ==================================================================================================================================================== */
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
//inline CMemory p_CClientState__CheckForResend = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x56\x57\x41\x57\x00\x81\xEC\x20\x04\x00\x00\x45\x0F\xB6\xF9\x00\x00\x00\x00\x8B\xF1\x48"), "xxxx?xxxx?xxxx?xxxxx????xxx"); /*48 89 5C 24 ?? 56 57 41 57 ?? 81 EC 20 04 ?? 00 45 0F B6 F9 ?? ?? ?? ?? 8B F1 48*/
//inline auto CClientState__CheckForResend = p_CClientState__CheckForResend.RCast<void(*)(CBaseClientState* thisptr, const char* a2, std::int64_t a3, char a4, int a5, std::uint8_t* a6)>();
#elif defined (GAMEDLL_S2)
//inline CMemory p_CClientState__CheckForResend = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x41\x56\x48\x81\xEC\x00\x00\x00\x00\x45\x0F\xB6"), "xxxx?xxxx?xxxx?xxxxx????xxx"); /*48 89 5C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 41 56 48 81 EC ?? ?? ?? ?? 45 0F B6*/
//inline auto CClientState__CheckForResend = p_CClientState__CheckForResend.RCast<void(*)(CBaseClientState* thisptr, const char* a2, std::int64_t a3, char a4, int a5, std::uint8_t* a6)>();
#elif defined (GAMEDLL_S3)
//inline CMemory p_CClientState__CheckForResend = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x41\x56\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\x32"), "xxxx?xxxx?xxxx?xxxxx????xxx"); /*48 89 5C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 41 56 48 81 EC ?? ?? ?? ?? 48 8B 32*/
//inline auto CClientState__CheckForResend = p_CClientState__CheckForResend.RCast<void(*)(CBaseClientState* thisptr, const char* a2, std::int64_t a3, char a4, int a5, std::uint8_t* a6)>();
#endif

///////////////////////////////////////////////////////////////////////////////
class HClientState : public IDetour
{
	virtual void GetAdr(void) const
	{
		//std::cout << "| FUN: CClientState::CheckForResend         : 0x" << std::hex << std::uppercase << p_CClientState__CheckForResend.GetPtr() << std::setw(nPad) << " |" << std::endl;
		std::cout << "| VAR: cl_m_bPaused                         : 0x" << std::hex << std::uppercase << cl_m_bPaused      << std::setw(0) << " |" << std::endl;
		std::cout << "| VAR: cl_host_tickcount                    : 0x" << std::hex << std::uppercase << cl_host_tickcount << std::setw(0) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const
	{

#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		CMemory localRef = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>(
			"\x40\x55\x48\x83\xEC\x50\x48\x8B\x05\x00\x00\x00\x00"), "xxxxxxxxx????");

		cl_m_bPaused = localRef.Offset(0x90)
			.FindPatternSelf("80 3D ? ? ? 0B ?", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x2, 0x2).RCast<bool*>();
		cl_host_tickcount = localRef.Offset(0xC0)
			.FindPatternSelf("66 0F 6E", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x4, 0x8).RCast<int*>();
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)

		CMemory localRef = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>(
			"\x40\x55\x48\x83\xEC\x30\x48\x8B\x05\x00\x00\x00\x00\x0F\xB6\xE9"), "xxxxxxxxx????xxx");

		cl_m_bPaused = localRef.Offset(0x70)
			.FindPatternSelf("80 3D ? ? ? 01 ?", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x2, 0x7).RCast<bool*>();
		cl_host_tickcount = localRef.Offset(0xC0)
			.FindPatternSelf("66 0F 6E", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x4, 0x8).RCast<int*>();
#endif
	}
	virtual void GetCon(void) const
	{
#ifndef DEDICATED
		g_pBaseClientState = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x0F\x84\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00\x48\x83\xC4\x28"), "xx????xxx????xxxx").FindPatternSelf("48 8D").ResolveRelativeAddressSelf(0x3, 0x7).RCast<CBaseClientState*>(); /*0F 84 ? ? ? ? 48 8D 0D ? ? ? ? 48 83 C4 28*/
#endif
	}
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HClientState);
