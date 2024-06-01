#pragma once
#include "public/ivrenderview.h"

inline void*(*V_DrawDepthOfField)(const float scalar);
inline void*(*V_DrawWorldMeshes)(void* baseEntity, void* renderContext, DrawWorldLists_t worldLists);
inline void*(*V_DrawWorldMeshesDepthOnly)(void* renderContext, DrawWorldLists_t worldLists);
inline void*(*V_DrawWorldMeshesDepthAtTheEnd)(void* ptr1, void* ptr2, void* ptr3, DrawWorldLists_t worldLists);

///////////////////////////////////////////////////////////////////////////////
class VGL_RSurf : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("R_DrawDepthOfField", V_DrawDepthOfField);
		LogFunAdr("R_DrawWorldMeshes", V_DrawWorldMeshes);
		LogFunAdr("R_DrawWorldMeshesDepthOnly", V_DrawWorldMeshesDepthOnly);
		LogFunAdr("R_DrawWorldMeshesDepthAtTheEnd", V_DrawWorldMeshesDepthAtTheEnd);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 83 EC 48 0F 28 E8").GetPtr(V_DrawDepthOfField);
		g_GameDll.FindPatternSIMD("48 8B C4 48 89 48 08 53 57 41 55").GetPtr(V_DrawWorldMeshes);
		g_GameDll.FindPatternSIMD("40 56 57 B8 ?? ?? ?? ??").GetPtr(V_DrawWorldMeshesDepthOnly);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 48 8B 0D ?? ?? ?? ?? 41 8B F9").GetPtr(V_DrawWorldMeshesDepthAtTheEnd);
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
