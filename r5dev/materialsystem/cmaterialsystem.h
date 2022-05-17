#pragma once

/* ==== MATERIALSYSTEM ================================================================================================================================================== */
inline CMemory p_CMaterialSystem__Init;
inline auto CMaterialSystem__Init = p_CMaterialSystem__Init.RCast<void* (*)(void* thisptr)>();

inline void* g_pMaterialSystem = nullptr;
#ifndef DEDICATED
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
inline CMemory P_DrawFrame;
inline auto V_DrawFrame = P_DrawFrame.RCast<void* (*)(int64_t a1, uint64_t a2, int a3, int a4, char a5, int a6, uint_8t a7, int64_t a8, uint32_t a9, uint32_t a10, __m128* a11, int a12)>();
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
inline CMemory p_DispatchDrawCall;
inline auto v_DispatchDrawCall = p_DispatchDrawCall.RCast<void* (*)(int64_t a1, uint64_t a2, int a3, int a4, int64_t a5, int a6, uint8_t a7, int64_t a8, uint32_t a9, uint32_t a10, int a11, __m128* a12, int a13, int64_t a14)>();
#endif

inline CMemory p_DrawStreamOverlay;
inline auto DrawStreamOverlay = p_DrawStreamOverlay.RCast<const char* (*)(void* thisptr, uint8_t* a2, void* unused, void* a4)>();

inline CMemory p_StreamDB_Init;
inline auto StreamDB_Init = p_StreamDB_Init.RCast<void (*)(const char* pszStreamDbFile)>();

inline CMemory s_pRenderContext;

inline int* total_streaming_tex_memory    = nullptr;
inline int* unfree_streaming_tex_memory   = nullptr;
inline int* unusable_streaming_tex_memory = nullptr;
#endif // !DEDICATED

void CMaterialSystem_Attach();
void CMaterialSystem_Detach();
///////////////////////////////////////////////////////////////////////////////
class VMaterialSystem : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: CMaterialSystem::Init                : {:#18x} |\n", p_CMaterialSystem__Init.GetPtr());
#ifndef DEDICATED
		spdlog::debug("| FUN: DispatchDrawCall                     : {:#18x} |\n", p_DispatchDrawCall.GetPtr());
		spdlog::debug("| FUN: DrawStreamOverlay                    : {:#18x} |\n", p_DrawStreamOverlay.GetPtr());
		spdlog::debug("| FUN: StreamDB_Init                        : {:#18x} |\n", p_StreamDB_Init.GetPtr());
		spdlog::debug("| VAR: s_pRenderContext                     : {:#18x} |\n", s_pRenderContext.GetPtr());
#endif // !DEDICATED
		spdlog::debug("| VAR: g_pMaterialSystem                    : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_pMaterialSystem));
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		p_CMaterialSystem__Init = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x55\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x83\xEC\x70\x48\x83\x3D\x00\x00\x00\x00\x00"), "xxxx?xxxxxxxxxxxxxxxxxx?????");
		CMaterialSystem__Init = p_CMaterialSystem__Init.RCast<void* (*)(void*)>(); /*48 89 5C 24 ?? 55 56 57 41 54 41 55 41 56 41 57 48 83 EC 70 48 83 3D ?? ?? ?? ?? ??*/
#ifndef DEDICATED
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_DispatchDrawCall = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x44\x89\x4C\x24\x00\x44\x89\x44\x24\x00\x48\x89\x4C\x24\x00\x55\x53"), "xxxx?xxxx?xxxx?xx");
		v_DispatchDrawCall = p_DispatchDrawCall.RCast<void* (*)(int64_t, uint64_t, int, int, char, int, uint8_t, int64_t, uint32_t, uint32_t, __m128*, int)>();
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_DispatchDrawCall = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x44\x89\x4C\x24\x00\x44\x89\x44\x24\x00\x48\x89\x4C\x24\x00\x55\x53\x56"), "xxxx?xxxx?xxxx?xxx");
		v_DispatchDrawCall = p_DispatchDrawCall.RCast<void* (*)(int64_t, uint64_t, int, int, int64_t, int, uint8_t, int64_t, uint32_t, uint32_t, int, __m128*, int, int64_t )>();
#endif
		p_DrawStreamOverlay = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x41\x56\xB8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x2B\xE0\xC6\x02\x00"), "xxx????x????xxxxxx");
		DrawStreamOverlay = p_DrawStreamOverlay.RCast<const char* (*)(void*, uint8_t*, void*, void*)>(); // 41 56 B8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 2B E0 C6 02 00 //

		p_StreamDB_Init = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x41\x54\x41\x56\x41\x57\x48\x83\xEC\x40\x48\x8B\xE9"), "xxxx?xxxx?xxxx?xxxx?xxxxxxxxxxxxx");
		StreamDB_Init = p_StreamDB_Init.RCast<void (*)(const char*)>(); /*48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 41 54 41 56 41 57 48 83 EC 40 48 8B E9*/
#endif // !DEDICATED
	}
	virtual void GetVar(void) const
	{
		g_pMaterialSystem = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>(
			"\x48\x8B\x0D\x00\x00\x00\x00\x48\x85\xC9\x74\x11\x48\x8B\x01\x48\x8D\x15\x00\x00\x00\x00"), "xxx????xxxxxxxxxxx????").ResolveRelativeAddressSelf(0x3, 0x7).RCast<void*>();
#ifndef DEDICATED
		s_pRenderContext = p_DispatchDrawCall.FindPattern("48 8B ?? ?? ?? ?? 01").ResolveRelativeAddressSelf(0x3, 0x7);

		total_streaming_tex_memory = p_DrawStreamOverlay.Offset(0x0).FindPatternSelf("48 8B 05", CMemory::Direction::DOWN).ResolveRelativeAddressSelf(0x3, 0x7).RCast<int*>();
		unfree_streaming_tex_memory = p_DrawStreamOverlay.Offset(0x20).FindPatternSelf("48 8B 05", CMemory::Direction::DOWN).ResolveRelativeAddressSelf(0x3, 0x7).RCast<int*>();
		unusable_streaming_tex_memory = p_DrawStreamOverlay.Offset(0x50).FindPatternSelf("48 8B 05", CMemory::Direction::DOWN).ResolveRelativeAddressSelf(0x3, 0x7).RCast<int*>();
#endif // !DEDICATED
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VMaterialSystem);
