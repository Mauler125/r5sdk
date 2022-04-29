//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: render surface
//
// $NoKeywords: $
//===========================================================================//
#include "core/stdafx.h"
#include "tier1/cvar.h"
#include "engine/gl_rsurf.h"

void* R_DrawWorldMeshes(void* baseEntity, void* renderContext, DrawWorldLists_t worldLists)
{
	if (r_drawWorldMeshes->GetBool())
		return V_DrawWorldMeshes(baseEntity, renderContext, worldLists);
	else
		return nullptr;
}

void* R_DrawWorldMeshesDepthOnly(void* renderContext, DrawWorldLists_t worldLists)
{
	if (r_drawWorldMeshesDepthOnly->GetBool())
		return V_DrawWorldMeshesDepthOnly(renderContext, worldLists);
	else
		return nullptr;
}

void* R_DrawWorldMeshesDepthAtTheEnd(void* ptr1, void* ptr2, void* ptr3, DrawWorldLists_t worldLists)
{
	if (r_drawWorldMeshesDepthAtTheEnd->GetBool())
		return V_DrawWorldMeshesDepthAtTheEnd(ptr1, ptr2, ptr3, worldLists);
	else
		return nullptr;
}

void RSurf_Attach()
{
	DetourAttach((LPVOID*)&V_DrawWorldMeshes, &R_DrawWorldMeshes);
	DetourAttach((LPVOID*)&V_DrawWorldMeshesDepthOnly, &R_DrawWorldMeshesDepthOnly);
	DetourAttach((LPVOID*)&V_DrawWorldMeshesDepthAtTheEnd, &R_DrawWorldMeshesDepthAtTheEnd);
}

void RSurf_Detach()
{
	DetourDetach((LPVOID*)&V_DrawWorldMeshes, &R_DrawWorldMeshes);
	DetourDetach((LPVOID*)&V_DrawWorldMeshesDepthOnly, &R_DrawWorldMeshesDepthOnly);
	DetourDetach((LPVOID*)&V_DrawWorldMeshesDepthAtTheEnd, &R_DrawWorldMeshesDepthAtTheEnd);
}