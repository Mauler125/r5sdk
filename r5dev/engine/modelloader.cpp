//===========================================================================//
//
// Purpose: Model loading / unloading interface
//
// $NoKeywords: $
//===========================================================================//

#include "core/stdafx.h"
#include "engine/cmodel_bsp.h"
#include "engine/modelloader.h"

uint64_t HCModelLoader__Map_LoadModelGuts(void* thisptr, void* mod)
{
	return CModelLoader__Map_LoadModelGuts(thisptr, mod);
}

///////////////////////////////////////////////////////////////////////////////
void CModelLoader_Attach()
{
	DetourAttach((LPVOID*)&CModelLoader__Map_LoadModelGuts, &HCModelLoader__Map_LoadModelGuts);
}

void CModelLoader_Detach()
{
	DetourDetach((LPVOID*)&CModelLoader__Map_LoadModelGuts, &HCModelLoader__Map_LoadModelGuts);
}