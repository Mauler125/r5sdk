#pragma once

//-------------------------------------------------------------------------
// CSTUDIORENDERCONTEXT
//-------------------------------------------------------------------------
inline void (*CStudioRenderContext__LoadModel)(__int64 thisptr, studiohdr_t* pStudioHdr, void* pVtxBuffer, studiohwdata_t* pStudioHWData);
inline __int64 (*CStudioRenderContext__LoadMaterials)(__int64 thisptr, studiohdr_t* pStudioHdr, void* a3, uint32_t* pFlags);

///////////////////////////////////////////////////////////////////////////////
class VStudioRenderContext : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CStudioRenderContext::LoadModel", CStudioRenderContext__LoadModel);
		LogFunAdr("CStudioRenderContext::LoadMaterials", CStudioRenderContext__LoadMaterials);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("4C 89 44 24 ?? 48 89 54 24 ?? 48 89 4C 24 ?? 53 55 56 57 48 83 EC 78").GetPtr(CStudioRenderContext__LoadModel);
		g_GameDll.FindPatternSIMD("48 8B C4 4C 89 40 18 55 56 41 55").GetPtr(CStudioRenderContext__LoadMaterials);
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const { }
};
///////////////////////////////////////////////////////////////////////////////
