//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: render surface
//
// $NoKeywords: $
//===========================================================================//
#include "core/stdafx.h"
#include "tier1/cvar.h"
#include "windows/id3dx.h"
#include "geforce/reflex.h"
#include "engine/gl_rsurf.h"
#include <materialsystem/cmaterialsystem.h>

static ConVar r_drawWorldMeshes("r_drawWorldMeshes", "1", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Render world meshes.");
static ConVar r_drawWorldMeshesDepthOnly("r_drawWorldMeshesDepthOnly", "1", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Render world meshes (depth only).");
static ConVar r_drawWorldMeshesDepthAtTheEnd("r_drawWorldMeshesDepthAtTheEnd", "1", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Render world meshes (depth at the end).");

void* R_DrawDepthOfField(const float scalar)
{
	GFX_SetLatencyMarker(D3D11Device(), RENDERSUBMIT_START, MaterialSystem()->GetCurrentFrameCount());
	return V_DrawDepthOfField(scalar);
}

void* R_DrawWorldMeshes(void* baseEntity, void* renderContext, DrawWorldLists_t worldLists)
{
	if (r_drawWorldMeshes.GetBool())
		return V_DrawWorldMeshes(baseEntity, renderContext, worldLists);
	else
		return nullptr;
}

void* R_DrawWorldMeshesDepthOnly(void* renderContext, DrawWorldLists_t worldLists)
{
	if (r_drawWorldMeshesDepthOnly.GetBool())
		return V_DrawWorldMeshesDepthOnly(renderContext, worldLists);
	else
		return nullptr;
}

void* R_DrawWorldMeshesDepthAtTheEnd(void* ptr1, void* ptr2, void* ptr3, DrawWorldLists_t worldLists)
{
	if (r_drawWorldMeshesDepthAtTheEnd.GetBool())
		return V_DrawWorldMeshesDepthAtTheEnd(ptr1, ptr2, ptr3, worldLists);
	else
		return nullptr;
}

void VGL_RSurf::Detour(const bool bAttach) const
{
	DetourSetup(&V_DrawDepthOfField, &R_DrawDepthOfField, bAttach);
	DetourSetup(&V_DrawWorldMeshes, &R_DrawWorldMeshes, bAttach);
	DetourSetup(&V_DrawWorldMeshesDepthOnly, &R_DrawWorldMeshesDepthOnly, bAttach);
	DetourSetup(&V_DrawWorldMeshesDepthAtTheEnd, &R_DrawWorldMeshesDepthAtTheEnd, bAttach);
}
