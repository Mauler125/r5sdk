//===========================================================================//
//
// Purpose: 
//
//===========================================================================//
#include "core/stdafx.h"
#include "tier0/crashhandler.h"
#include "tier1/cvar.h"
#include "vpc/keyvalues.h"
#include "rtech/rtech_utils.h"
#include "engine/cmodel_bsp.h"
#ifndef MATERIALSYSTEM_NODX
#include "materialsystem/cmaterialglue.h"
#endif // !MATERIALSYSTEM_NODX
#include "materialsystem/cmaterialsystem.h"

//-----------------------------------------------------------------------------
// Purpose: initialization of the material system
//-----------------------------------------------------------------------------
InitReturnVal_t CMaterialSystem::Init(CMaterialSystem* thisptr)
{
#ifdef MATERIALSYSTEM_NODX
	// Only load the 'startup.rpak' file, as 'common_early.rpak' has assets
	// that references assets in 'startup.rpak'.
	PakHandle_t pakHandle = g_pakLoadApi->LoadAsync("startup.rpak", AlignedMemAlloc(), 5, 0);
	g_pakLoadApi->WaitAsync(pakHandle, nullptr);

	// Trick: return INIT_FAILED to disable the loading of hardware
	// configuration data, since we don't need it on the dedi.
	return INIT_FAILED;
#else
	// Initialize as usual.
	return CMaterialSystem__Init(thisptr);
#endif
}

#ifndef MATERIALSYSTEM_NODX
//---------------------------------------------------------------------------------
// Purpose: loads and processes STBSP files
// (overrides level name if stbsp field has value in prerequisites file)
// Input  : *pszLevelName - 
//---------------------------------------------------------------------------------
void StreamDB_Init(const char* pszLevelName)
{
	KeyValues* pSettingsKV = Mod_GetLevelSettings(pszLevelName);

	if (pSettingsKV)
	{
		KeyValues* pStreamKV = pSettingsKV->FindKey("StreamDB");

		if (pStreamKV)
		{
			const char* pszColumnName = pStreamKV->GetString();
			Msg(eDLL_T::MS, "StreamDB_Init: Loading override STBSP file '%s.stbsp'\n", pszColumnName);

			v_StreamDB_Init(pszColumnName);
			return;
		}
	}

	Msg(eDLL_T::MS, "StreamDB_Init: Loading STBSP file '%s.stbsp'\n", pszLevelName);
	v_StreamDB_Init(pszLevelName);
}

//---------------------------------------------------------------------------------
// Purpose: draw frame
//---------------------------------------------------------------------------------
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
void* __fastcall DispatchDrawCall(int64_t a1, uint64_t a2, int a3, int a4, char a5, int a6, uint8_t a7, int64_t a8, uint32_t a9, uint32_t a10, __m128* a11, int a12)
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
void* __fastcall DispatchDrawCall(int64_t a1, uint64_t a2, int a3, int a4, int64_t a5, int a6, uint8_t a7, int64_t a8, uint32_t a9, uint32_t a10, int a11, __m128* a12, int a13, int64_t a14)
#endif
{
	// This only happens when the BSP is in a horrible condition (bad depth buffer draw calls!)
	// but allows you to load BSP's with virtually all missing shaders/materials and models 
	// being replaced with 'material_for_aspect/error.rpak' and 'mdl/error.rmdl'.
	if (!s_pRenderContext.GetValue<void*>())
		return nullptr;
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	return v_DispatchDrawCall(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12);
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	return v_DispatchDrawCall(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14);
#endif
}
#endif // !MATERIALSYSTEM_NODX

#ifndef MATERIALSYSTEM_NODX
//-----------------------------------------------------------------------------
// Purpose: finds a material
// Input  : *pMatSys - 
//			*pMaterialName - 
//			nMaterialType - 
//			nUnk - 
//			bComplain - 
// Output : pointer to material
//-----------------------------------------------------------------------------
CMaterialGlue* CMaterialSystem::FindMaterialEx(CMaterialSystem* pMatSys, const char* pMaterialName, uint8_t nMaterialType, int nUnk, bool bComplain)
{
	CMaterialGlue* pMaterial = CMaterialSystem__FindMaterialEx(pMatSys, pMaterialName, nMaterialType, nUnk, bComplain);

	if ((bComplain || mat_alwaysComplain->GetBool()) && pMaterial->IsErrorMaterial())
	{
		Error(eDLL_T::MS, NO_ERROR, "Material \"%s\" not found; replacing with \"%s\".\n", pMaterialName, pMaterial->GetName());
	}
	return pMaterial;
}

//-----------------------------------------------------------------------------
// Purpose: get screen size
// Input  : *pMatSys - 
// Output : Vector2D screen size
//-----------------------------------------------------------------------------
Vector2D CMaterialSystem::GetScreenSize(CMaterialSystem* pMatSys)
{
	Vector2D vecScreenSize;

	CMaterialSystem_GetScreenSize(pMatSys, &vecScreenSize.x, &vecScreenSize.y);

	return vecScreenSize;
}
#endif // !MATERIALSYSTEM_NODX

///////////////////////////////////////////////////////////////////////////////
void VMaterialSystem::Detour(const bool bAttach) const
{
	DetourSetup(&CMaterialSystem__Init, &CMaterialSystem::Init, bAttach);
#ifndef MATERIALSYSTEM_NODX
	DetourSetup(&v_StreamDB_Init, &StreamDB_Init, bAttach);
	DetourSetup(&v_DispatchDrawCall, &DispatchDrawCall, bAttach);
	DetourSetup(&CMaterialSystem__FindMaterialEx, &CMaterialSystem::FindMaterialEx, bAttach);
#endif // !MATERIALSYSTEM_NODX
}
