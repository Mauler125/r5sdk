#ifndef MATERIALSYSTEM_H
#define MATERIALSYSTEM_H
#include "cmaterialglue.h"
#include "public/imaterialsystem.h"

#define STREAM_DB_EXT "stbsp"

class CMaterialSystem
{
public:
	static InitReturnVal_t Init(CMaterialSystem* thisptr);
#ifndef MATERIALSYSTEM_NODX
	static CMaterialGlue* FindMaterialEx(CMaterialSystem* pMatSys, const char* pMaterialName, uint8_t nMaterialType, int nUnk, bool bComplain);
	static Vector2D GetScreenSize(CMaterialSystem* pMatSys = nullptr);
#endif // !MATERIALSYSTEM_NODX
};

#ifndef MATERIALSYSTEM_NODX
class CMaterialDeviceMgr
{
public:
	inline const MaterialAdapterInfo_t& GetAdapterInfo(int nIndex) const
	{
		Assert(nIndex >= 0 && nIndex < SDK_ARRAYSIZE(m_AdapterInfo));
		return m_AdapterInfo[nIndex];
	}
	inline const MaterialAdapterInfo_t& GetAdapterInfo() const
	{
		// Retrieve info of the selected adapter.
		return GetAdapterInfo(m_SelectedAdapter);
	}

private:
	enum
	{
		MAX_ADAPTER_COUNT = 4
	};

	IDXGIAdapter* m_Adapters[MAX_ADAPTER_COUNT];
	void* m_pUnknown1[MAX_ADAPTER_COUNT];
	MaterialAdapterInfo_t m_AdapterInfo[MAX_ADAPTER_COUNT];
	size_t m_AdapterMemorySize[MAX_ADAPTER_COUNT];
	int m_NumDisplayAdaptersProcessed;
	int m_SelectedAdapter;
	int m_NumDisplayAdapters;
};

inline CMaterialDeviceMgr* g_pMaterialAdapterMgr = nullptr;
#endif // !MATERIALSYSTEM_NODX

/* ==== MATERIALSYSTEM ================================================================================================================================================== */
inline CMemory p_CMaterialSystem__Init;
inline InitReturnVal_t(*CMaterialSystem__Init)(CMaterialSystem* thisptr);

inline CMemory p_CMaterialSystem__Disconnect;
inline void(*CMaterialSystem__Disconnect)(void);

inline CMaterialSystem* g_pMaterialSystem = nullptr;
inline void* g_pMaterialVFTable = nullptr;
#ifndef MATERIALSYSTEM_NODX
inline CMemory p_CMaterialSystem__FindMaterialEx;
inline CMaterialGlue*(*CMaterialSystem__FindMaterialEx)(CMaterialSystem* pMatSys, const char* pMaterialName, uint8_t nMaterialType, int nUnk, bool bComplain);

inline CMemory p_CMaterialSystem_GetScreenSize;
inline void(*CMaterialSystem_GetScreenSize)(CMaterialSystem* pMatSys, float* outX, float* outY);

#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
inline CMemory p_DispatchDrawCall;
inline void*(*v_DispatchDrawCall)(int64_t a1, uint64_t a2, int a3, int a4, char a5, int a6, uint8_t a7, int64_t a8, uint32_t a9, uint32_t a10, __m128* a11, int a12);
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
inline CMemory p_DispatchDrawCall;
inline void*(*v_DispatchDrawCall)(int64_t a1, uint64_t a2, int a3, int a4, int64_t a5, int a6, uint8_t a7, int64_t a8, uint32_t a9, uint32_t a10, int a11, __m128* a12, int a13, int64_t a14);
#endif
inline CMemory p_GetStreamOverlay;
inline void(*v_GetStreamOverlay)(const char* mode, char* buf, size_t bufSize);

inline CMemory p_DrawStreamOverlay;
inline const char*(*v_DrawStreamOverlay)(void* thisptr, uint8_t* a2, void* unused, void* a4);

inline CMemory s_pRenderContext;

inline int* g_nTotalStreamingTextureMemory    = nullptr;
inline int* g_nUnfreeStreamingTextureMemory   = nullptr;
inline int* g_nUnusableStreamingTextureMemory = nullptr;
#endif // !MATERIALSYSTEM_NODX

///////////////////////////////////////////////////////////////////////////////
class VMaterialSystem : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogConAdr("CMaterial::`vftable'", reinterpret_cast<uintptr_t>(g_pMaterialVFTable));
		LogFunAdr("CMaterialSystem::Init", p_CMaterialSystem__Init.GetPtr());
		LogFunAdr("CMaterialSystem::Disconnect", p_CMaterialSystem__Disconnect.GetPtr());
#ifndef MATERIALSYSTEM_NODX
		LogFunAdr("CMaterialSystem::FindMaterialEx", p_CMaterialSystem__FindMaterialEx.GetPtr());
		LogFunAdr("CMaterialSystem::GetScreenSize", p_CMaterialSystem_GetScreenSize.GetPtr());
		LogFunAdr("CMaterialSystem::DispatchDrawCall", p_DispatchDrawCall.GetPtr());
		LogFunAdr("CMaterialSystem::GetStreamOverlay", p_GetStreamOverlay.GetPtr());
		LogFunAdr("CMaterialSystem::DrawStreamOverlay", p_DrawStreamOverlay.GetPtr());
		LogVarAdr("g_nTotalStreamingTextureMemory", reinterpret_cast<uintptr_t>(g_nTotalStreamingTextureMemory));
		LogVarAdr("g_nUnfreeStreamingTextureMemory", reinterpret_cast<uintptr_t>(g_nUnfreeStreamingTextureMemory));
		LogVarAdr("g_nUnusableStreamingTextureMemory", reinterpret_cast<uintptr_t>(g_nUnusableStreamingTextureMemory));
		LogVarAdr("s_pRenderContext", s_pRenderContext.GetPtr());
		LogVarAdr("g_MaterialAdapterMgr", reinterpret_cast<uintptr_t>(g_pMaterialAdapterMgr));
#endif // !MATERIALSYSTEM_NODX
		LogVarAdr("g_pMaterialSystem", reinterpret_cast<uintptr_t>(g_pMaterialSystem));
	}
	virtual void GetFun(void) const
	{
		p_CMaterialSystem__Init = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 55 56 57 41 54 41 55 41 56 41 57 48 83 EC 70 48 83 3D ?? ?? ?? ?? ??");
		CMaterialSystem__Init = p_CMaterialSystem__Init.RCast<InitReturnVal_t(*)(CMaterialSystem*)>(); /*48 89 5C 24 ?? 55 56 57 41 54 41 55 41 56 41 57 48 83 EC 70 48 83 3D ?? ?? ?? ?? ??*/

		p_CMaterialSystem__Disconnect = g_GameDll.FindPatternSIMD("48 83 EC 28 8B 0D ?? ?? ?? ?? 48 89 6C 24 ??");
		CMaterialSystem__Disconnect = p_CMaterialSystem__Disconnect.RCast<void(*)(void)>();
#ifndef MATERIALSYSTEM_NODX
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
		p_GetStreamOverlay = g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 80 7C 24 ?? ?? 0F 84 ?? ?? ?? ?? 48 89 9C 24 ?? ?? ?? ??").FollowNearCallSelf();
		v_GetStreamOverlay = p_GetStreamOverlay.RCast<void(*)(const char*, char*, size_t)>(); /*E8 ? ? ? ? 80 7C 24 ? ? 0F 84 ? ? ? ? 48 89 9C 24 ? ? ? ?*/

		p_DrawStreamOverlay = g_GameDll.FindPatternSIMD("41 56 B8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 2B E0 C6 02 ??");
		v_DrawStreamOverlay = p_DrawStreamOverlay.RCast<const char*(*)(void*, uint8_t*, void*, void*)>(); // 41 56 B8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 2B E0 C6 02 00 //
#endif // !MATERIALSYSTEM_NODX
	}
	virtual void GetVar(void) const
	{
#ifndef MATERIALSYSTEM_NODX
		g_nTotalStreamingTextureMemory = p_DrawStreamOverlay.Offset(0x1C).FindPatternSelf("48 8B 05", CMemory::Direction::DOWN).ResolveRelativeAddressSelf(0x3, 0x7).RCast<int*>();
		g_nUnfreeStreamingTextureMemory = p_DrawStreamOverlay.Offset(0x2D).FindPatternSelf("48 8B 05", CMemory::Direction::DOWN).ResolveRelativeAddressSelf(0x3, 0x7).RCast<int*>();
		g_nUnusableStreamingTextureMemory = p_DrawStreamOverlay.Offset(0x50).FindPatternSelf("48 8B 05", CMemory::Direction::DOWN).ResolveRelativeAddressSelf(0x3, 0x7).RCast<int*>();

		s_pRenderContext = p_DispatchDrawCall.FindPattern("48 8B ?? ?? ?? ?? 01").ResolveRelativeAddressSelf(0x3, 0x7);
		g_pMaterialAdapterMgr = p_CMaterialSystem__Disconnect.FindPattern("48 8D").ResolveRelativeAddressSelf(0x3, 0x7).RCast<CMaterialDeviceMgr*>();
#endif // !MATERIALSYSTEM_NODX
		g_pMaterialSystem = g_GameDll.FindPatternSIMD("48 8B 0D ?? ?? ?? ?? 48 85 C9 74 11 48 8B 01 48 8D 15 ?? ?? ?? ??").ResolveRelativeAddressSelf(0x3, 0x7).RCast<CMaterialSystem*>();
	}
	virtual void GetCon(void) const
	{
		g_pMaterialVFTable = g_GameDll.GetVirtualMethodTable(".?AVCMaterial@@").RCast<void*>();
	}
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////

#endif // MATERIALSYSTEM_H
