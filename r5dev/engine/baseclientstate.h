#pragma once
#include "engine/debugoverlay.h"

#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
inline bool* cl_m_bPaused = p_DrawAllOverlays.Offset(0x90).FindPatternSelf("80 3D ? ? ? 0B ?", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x2, 0x2).RCast<bool*>();
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
inline bool* cl_m_bPaused = p_DrawAllOverlays.Offset(0x70).FindPatternSelf("80 3D ? ? ? 01 ?", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x2, 0x7).RCast<bool*>();
#endif
inline int* cl_host_tickcount = p_DrawAllOverlays.Offset(0xC0).FindPatternSelf("66 0F 6E", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x4, 0x8).RCast<int*>();

///////////////////////////////////////////////////////////////////////////////
class CBaseClientState
{
public:
	bool* m_bPaused = cl_m_bPaused; // pauzes the client side simulation in apex.
	int* host_tickcount = cl_host_tickcount; // client simulation tick count.

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
	virtual void debugp()
	{
		//std::cout << "| FUN: CClientState::CheckForResend         : 0x" << std::hex << std::uppercase << p_CClientState__CheckForResend.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "| VAR: cl_m_bPaused                         : 0x" << std::hex << std::uppercase << cl_m_bPaused      << std::setw(0) << " |" << std::endl;
		std::cout << "| FUN: cl_host_tickcount                    : 0x" << std::hex << std::uppercase << cl_host_tickcount << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HClientState);
