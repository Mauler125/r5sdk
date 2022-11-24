//===========================================================================//
//
// Purpose: 
//
//===========================================================================//
#include "core/stdafx.h"
#include "tier1/cvar.h"
#include "vpc/keyvalues.h"
#include "rtech/rtech_utils.h"
#include "engine/cmodel_bsp.h"
#include "materialsystem/cmaterialglue.h"
#include "materialsystem/cmaterialsystem.h"

#ifndef DEDICATED
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
			DevMsg(eDLL_T::MS, "%s: Loading override STBSP file '%s.stbsp'\n", __FUNCTION__, pszColumnName);

			v_StreamDB_Init(pszColumnName);
			return;
		}
	}

	DevMsg(eDLL_T::MS, "%s: Loading STBSP file '%s.stbsp'\n", __FUNCTION__, pszLevelName);
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
#endif // !DEDICATED

//-----------------------------------------------------------------------------
// Purpose: checks if ptr is valid, and checks for equality against CMaterial vftable
// Input  : **pCandidate - 
// Output : true if valid and material, false otherwise
//-----------------------------------------------------------------------------
bool IsMaterialInternal(void** pCandidate)
{
	// NOTE: this is a dirty fix, but for running technically broken BSP's, this is the only fix 
	// besides going bare metal inline assembly (which on its own isn't directly the problem, but 
	// portability wise it will be a problem as the majority of the code in r5apex.exe is declared inline).
	// In the future, do not fix anything like this unless there is absolutely no other choice!
	// The context of the problem is that we fix the missing models defined in the game_lump of a 
	// BSP by swapping missing models out for existing models, which will in many cases, end up with 
	// 2 or more model name duplicates within a single BSP's game_lump, which is illegal and causes 
	// unpredictable behavior, which in this case causes a register to be assigned to an invalid CMaterial
	// address. The pointer can still be dereferenced in many cases, which is why we do an equality test.
	// The first member of the CMaterial data structure should be its VFTable pointer, anything else is invalid.
	__try
	{
		if (*pCandidate == g_pMaterialVFTable ||
			*pCandidate == g_pMaterialGlueVFTable)
			return true;
	}
	__except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION)
	{
		return false;
	}
	return false;
}

#ifndef DEDICATED
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
#endif // !DEDICATED

///////////////////////////////////////////////////////////////////////////////
void CMaterialSystem_Attach()
{
#ifndef DEDICATED
	DetourAttach((LPVOID*)&v_StreamDB_Init, &StreamDB_Init);
	DetourAttach((LPVOID*)&v_DispatchDrawCall, &DispatchDrawCall);
	DetourAttach((LPVOID*)&CMaterialSystem__FindMaterialEx, &CMaterialSystem::FindMaterialEx);
#endif // !DEDICATED
}

void CMaterialSystem_Detach()
{
#ifndef DEDICATED
	DetourDetach((LPVOID*)&v_StreamDB_Init, &StreamDB_Init);
	DetourDetach((LPVOID*)&v_DispatchDrawCall, &DispatchDrawCall);
	DetourDetach((LPVOID*)&CMaterialSystem__FindMaterialEx, &CMaterialSystem::FindMaterialEx);
#endif // !DEDICATED
}