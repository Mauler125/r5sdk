#pragma once
#include "public/ivrenderview.h"

inline CMemory P_DrawWorldMeshes;
inline void*(*V_DrawWorldMeshes)(void* baseEntity, void* renderContext, DrawWorldLists_t worldLists);

inline CMemory P_DrawWorldMeshesDepthOnly;
inline void*(*V_DrawWorldMeshesDepthOnly)(void* renderContext, DrawWorldLists_t worldLists);

inline CMemory P_DrawWorldMeshesDepthAtTheEnd;
inline void*(*V_DrawWorldMeshesDepthAtTheEnd)(void* ptr1, void* ptr2, void* ptr3, DrawWorldLists_t worldLists);

///////////////////////////////////////////////////////////////////////////////
class VGL_RSurf : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("R_DrawWorldMeshes", P_DrawWorldMeshes.GetPtr());
		LogFunAdr("R_DrawWorldMeshesDepthOnly", P_DrawWorldMeshesDepthOnly.GetPtr());
		LogFunAdr("R_DrawWorldMeshesDepthAtTheEnd", P_DrawWorldMeshesDepthAtTheEnd.GetPtr());
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		P_DrawWorldMeshes              = g_GameDll.FindPatternSIMD("48 8B C4 48 89 48 08 53 48 83 EC 70");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		P_DrawWorldMeshes              = g_GameDll.FindPatternSIMD("48 8B C4 48 89 48 08 53 57 41 55");
#endif
		P_DrawWorldMeshesDepthOnly     = g_GameDll.FindPatternSIMD("40 56 57 B8 ?? ?? ?? ??");
		P_DrawWorldMeshesDepthAtTheEnd = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 48 8B 0D ?? ?? ?? ?? 41 8B F9");

		V_DrawWorldMeshes              = P_DrawWorldMeshes.RCast<void* (*)(void*, void*, DrawWorldLists_t)>();                     /*48 8B C4 48 89 48 08 53 57 41 55*/
		V_DrawWorldMeshesDepthOnly     = P_DrawWorldMeshesDepthOnly.RCast<void* (*)(void*, DrawWorldLists_t)>();                   /*40 56 57 B8 ?? ?? ?? ??*/
		V_DrawWorldMeshesDepthAtTheEnd = P_DrawWorldMeshesDepthAtTheEnd.RCast<void* (*)(void*, void*, void*, DrawWorldLists_t)>(); /*48 89 5C 24 ?? 48 89 74 24 ? 57 48 83 EC 20 48 8B 0D ?? ?? ?? ?? 41 8B F9*/
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
