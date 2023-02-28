#ifndef MATERIALSYSTEM_H
#define MATERIALSYSTEM_H
#include "cmaterialglue.h"

#define STREAM_DB_EXT "stbsp"

class CMaterialSystem
{
public:
	FORCENOINLINE static bool IsMaterialInternal(void** pCandidate);
#ifndef DEDICATED
	static CMaterialGlue* FindMaterialEx(CMaterialSystem* pMatSys, const char* pMaterialName, uint8_t nMaterialType, int nUnk, bool bComplain);
	static Vector2D GetScreenSize(CMaterialSystem* pMatSys = nullptr);
#endif // !DEDICATED
};

/* ==== MATERIALSYSTEM ================================================================================================================================================== */
inline CMemory p_CMaterialSystem__Init;
inline auto CMaterialSystem__Init = p_CMaterialSystem__Init.RCast<void*(*)(CMaterialSystem* thisptr)>();

inline void* g_pMaterialSystem = nullptr;
inline void* g_pMaterialVFTable = nullptr;
#ifndef DEDICATED
inline CMemory p_CMaterialSystem__FindMaterialEx;
inline auto CMaterialSystem__FindMaterialEx = p_CMaterialSystem__FindMaterialEx.RCast<CMaterialGlue*(*)(CMaterialSystem* pMatSys, const char* pMaterialName, uint8_t nMaterialType, int nUnk, bool bComplain)>();

inline CMemory p_CMaterialSystem_GetScreenSize;
inline auto CMaterialSystem_GetScreenSize = p_CMaterialSystem_GetScreenSize.RCast<void(*)(CMaterialSystem* pMatSys, float* outX, float* outY)>();

#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
inline CMemory p_DispatchDrawCall;
inline auto v_DispatchDrawCall = p_DispatchDrawCall.RCast<void*(*)(int64_t a1, uint64_t a2, int a3, int a4, char a5, int a6, uint8_t a7, int64_t a8, uint32_t a9, uint32_t a10, __m128* a11, int a12)>();
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
inline CMemory p_DispatchDrawCall;
inline auto v_DispatchDrawCall = p_DispatchDrawCall.RCast<void*(*)(int64_t a1, uint64_t a2, int a3, int a4, int64_t a5, int a6, uint8_t a7, int64_t a8, uint32_t a9, uint32_t a10, int a11, __m128* a12, int a13, int64_t a14)>();
#endif
inline CMemory p_DrawStreamOverlay;
inline auto v_DrawStreamOverlay = p_DrawStreamOverlay.RCast<const char*(*)(void* thisptr, uint8_t* a2, void* unused, void* a4)>();

inline CMemory s_pRenderContext;

inline int* g_nTotalStreamingTextureMemory    = nullptr;
inline int* g_nUnfreeStreamingTextureMemory   = nullptr;
inline int* g_nUnusableStreamingTextureMemory = nullptr;
#endif // !DEDICATED

///////////////////////////////////////////////////////////////////////////////
class VMaterialSystem : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogConAdr("CMaterial::`vftable'", reinterpret_cast<uintptr_t>(g_pMaterialVFTable));
		LogFunAdr("CMaterialSystem::Init", p_CMaterialSystem__Init.GetPtr());
#ifndef DEDICATED
		LogFunAdr("CMaterialSystem::FindMaterialEx", p_CMaterialSystem__FindMaterialEx.GetPtr());
		LogFunAdr("CMaterialSystem::GetScreenSize", p_CMaterialSystem_GetScreenSize.GetPtr());
		LogFunAdr("CMaterialSystem::DispatchDrawCall", p_DispatchDrawCall.GetPtr());
		LogFunAdr("CMaterialSystem::DrawStreamOverlay", p_DrawStreamOverlay.GetPtr());
		LogVarAdr("g_nTotalStreamingTextureMemory", reinterpret_cast<uintptr_t>(g_nTotalStreamingTextureMemory));
		LogVarAdr("g_nUnfreeStreamingTextureMemory", reinterpret_cast<uintptr_t>(g_nUnfreeStreamingTextureMemory));
		LogVarAdr("g_nUnusableStreamingTextureMemory", reinterpret_cast<uintptr_t>(g_nUnusableStreamingTextureMemory));
		LogVarAdr("s_pRenderContext", s_pRenderContext.GetPtr());
#endif // !DEDICATED
		LogVarAdr("g_pMaterialSystem", reinterpret_cast<uintptr_t>(g_pMaterialSystem));
	}
	virtual void GetFun(void) const
	{
		p_CMaterialSystem__Init = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 55 56 57 41 54 41 55 41 56 41 57 48 83 EC 70 48 83 3D ?? ?? ?? ?? ??");
		CMaterialSystem__Init = p_CMaterialSystem__Init.RCast<void*(*)(CMaterialSystem*)>(); /*48 89 5C 24 ?? 55 56 57 41 54 41 55 41 56 41 57 48 83 EC 70 48 83 3D ?? ?? ?? ?? ??*/
#ifndef DEDICATED
		p_CMaterialSystem__FindMaterialEx = g_GameDll.FindPatternSIMD("44 89 4C 24 ?? 44 88 44 24 ?? 48 89 4C 24 ??");
		CMaterialSystem__FindMaterialEx = p_CMaterialSystem__FindMaterialEx.RCast<CMaterialGlue*(*)(CMaterialSystem*, const char*, uint8_t, int, bool)>(); /*44 89 4C 24 ?? 44 88 44 24 ?? 48 89 4C 24 ??*/

		p_CMaterialSystem_GetScreenSize = g_GameDll.FindPatternSIMD("8B 05 ?? ?? ?? ?? 89 02 8B 05 ?? ?? ?? ?? 41 89 ?? C3 CC CC CC CC CC CC CC CC CC CC CC CC CC CC 8B 05 ?? ?? ?? ??");
		CMaterialSystem_GetScreenSize = p_CMaterialSystem_GetScreenSize.RCast<void(*)(CMaterialSystem* pMatSys, float* outX, float* outY)>(); /*8B 05 ? ? ? ? 89 02 8B 05 ? ? ? ? 41 89 00 C3 CC CC CC CC CC CC CC CC CC CC CC CC CC CC 8B 05 ? ? ? ?*/
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_DispatchDrawCall = g_GameDll.FindPatternSIMD("44 89 4C 24 ?? 44 89 44 24 ?? 48 89 4C 24 ?? 55 53");
		v_DispatchDrawCall = p_DispatchDrawCall.RCast<void*(*)(int64_t, uint64_t, int, int, char, int, uint8_t, int64_t, uint32_t, uint32_t, __m128*, int)>();
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_DispatchDrawCall = g_GameDll.FindPatternSIMD("44 89 4C 24 ?? 44 89 44 24 ?? 48 89 4C 24 ?? 55 53 56");
		v_DispatchDrawCall = p_DispatchDrawCall.RCast<void*(*)(int64_t, uint64_t, int, int, int64_t, int, uint8_t, int64_t, uint32_t, uint32_t, int, __m128*, int, int64_t )>();
#endif
		p_DrawStreamOverlay = g_GameDll.FindPatternSIMD("41 56 B8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 2B E0 C6 02 ??");
		v_DrawStreamOverlay = p_DrawStreamOverlay.RCast<const char*(*)(void*, uint8_t*, void*, void*)>(); // 41 56 B8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 2B E0 C6 02 00 //
#endif // !DEDICATED
	}
	virtual void GetVar(void) const
	{
#ifndef DEDICATED
		g_nTotalStreamingTextureMemory = p_DrawStreamOverlay.Offset(0x1C).FindPatternSelf("48 8B 05", CMemory::Direction::DOWN).ResolveRelativeAddressSelf(0x3, 0x7).RCast<int*>();
		g_nUnfreeStreamingTextureMemory = p_DrawStreamOverlay.Offset(0x2D).FindPatternSelf("48 8B 05", CMemory::Direction::DOWN).ResolveRelativeAddressSelf(0x3, 0x7).RCast<int*>();
		g_nUnusableStreamingTextureMemory = p_DrawStreamOverlay.Offset(0x50).FindPatternSelf("48 8B 05", CMemory::Direction::DOWN).ResolveRelativeAddressSelf(0x3, 0x7).RCast<int*>();

		s_pRenderContext = p_DispatchDrawCall.FindPattern("48 8B ?? ?? ?? ?? 01").ResolveRelativeAddressSelf(0x3, 0x7);
#endif // !DEDICATED
		g_pMaterialSystem = g_GameDll.FindPatternSIMD("48 8B 0D ?? ?? ?? ?? 48 85 C9 74 11 48 8B 01 48 8D 15 ?? ?? ?? ??").ResolveRelativeAddressSelf(0x3, 0x7).RCast<void*>();
	}
	virtual void GetCon(void) const
	{
		g_pMaterialVFTable = g_GameDll.GetVirtualMethodTable(".?AVCMaterial@@").RCast<void*>();
	}
	virtual void Attach(void) const;
	virtual void Detach(void) const;
};
///////////////////////////////////////////////////////////////////////////////

#endif // MATERIALSYSTEM_H