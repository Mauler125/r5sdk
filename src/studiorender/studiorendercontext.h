#pragma once

//-------------------------------------------------------------------------
// CSTUDIORENDERCONTEXT
//-------------------------------------------------------------------------
inline CMemory CStudioRenderContext__LoadModel;
inline CMemory CStudioRenderContext__LoadMaterials;

///////////////////////////////////////////////////////////////////////////////
class VStudioRenderContext : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CStudioRenderContext::LoadModel", CStudioRenderContext__LoadModel.GetPtr());
		LogFunAdr("CStudioRenderContext::LoadMaterials", CStudioRenderContext__LoadMaterials.GetPtr());
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S1)
		CStudioRenderContext__LoadModel = g_GameDll.FindPatternSIMD("4C 89 44 24 ?? 53 55 56 41 54 41 57");
#elif defined (GAMEDLL_S2)
		CStudioRenderContext__LoadModel = g_GameDll.FindPatternSIMD("4C 89 44 24 ?? 48 89 54 24 ?? 53 57 41 55 48 81 EC ?? ?? ?? ??");
#elif defined (GAMEDLL_S3)
		CStudioRenderContext__LoadModel = g_GameDll.FindPatternSIMD("4C 89 44 24 ?? 48 89 54 24 ?? 48 89 4C 24 ?? 53 55 56 57 48 83 EC 78");
#endif// 0x1404554C0 // 4C 89 44 24 ? 48 89 54 24 ? 48 89 4C 24 ? 53 55 56 57 48 83 EC 78 //

#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		CStudioRenderContext__LoadMaterials = g_GameDll.FindPatternSIMD("4C 89 44 24 ?? 55 56 41 57");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		CStudioRenderContext__LoadMaterials = g_GameDll.FindPatternSIMD("48 8B C4 4C 89 40 18 55 56 41 55");
#endif// 0x140456B50 // 48 8B C4 4C 89 40 18 55 56 41 55 //
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const { }
};
///////////////////////////////////////////////////////////////////////////////
