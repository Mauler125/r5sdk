//=============================================================================//
//
// Purpose: RTech game utilities
//
//=============================================================================//
#include "core/stdafx.h"
#include "engine/host_cmd.h"
#include "engine/host_state.h"
#include "engine/cmodel_bsp.h"
#include "rtech/rtech_game.h"
#include "rtech/rtech_utils.h"

// Pak handles that have been loaded with the level
// from within the level settings KV (located in
// scripts/levels/settings/*.kv). On level unload,
// each pak listed in this vector gets unloaded.
CUtlVector<RPakHandle_t> g_vLoadedPakHandle;

//-----------------------------------------------------------------------------
// Purpose: load user-requested pak files on-demand
// Input  : *szPakFileName - 
//			*pMalloc - 
//			nIdx - 
//			bUnk - 
// Output : pak file handle on success, INVALID_PAK_HANDLE on failure
//-----------------------------------------------------------------------------
RPakHandle_t Pak_LoadAsync(const char* szPakFileName, CAlignedMemAlloc* pMalloc, int nIdx, bool bUnk)
{
	RPakHandle_t pakHandle = INVALID_PAK_HANDLE;

	CUtlString pakBasePath;
	CUtlString pakOverridePath;

	pakBasePath.Format(PLATFORM_PAK_PATH "%s", szPakFileName);
	pakOverridePath.Format(PLATFORM_PAK_OVERRIDE_PATH "%s", szPakFileName);

	if (FileExists(pakOverridePath.Get()) || FileExists(pakBasePath.Get()))
	{
		Msg(eDLL_T::RTECH, "Loading pak file: '%s'\n", szPakFileName);
		pakHandle = v_Pak_LoadAsync(szPakFileName, pMalloc, nIdx, bUnk);

		if (pakHandle == INVALID_PAK_HANDLE)
		{
			Error(eDLL_T::RTECH, NO_ERROR, "%s: Failed read '%s' results '%u'\n", __FUNCTION__, szPakFileName, pakHandle);
		}
	}
	else
	{
		Error(eDLL_T::RTECH, NO_ERROR, "%s: Failed; file '%s' doesn't exist\n", __FUNCTION__, szPakFileName);
	}

	return pakHandle;
}

//-----------------------------------------------------------------------------
// Purpose: unloads loaded pak files
// Input  : handle - 
//-----------------------------------------------------------------------------
void Pak_UnloadPak(RPakHandle_t handle)
{
	RPakLoadedInfo_t* pakInfo = g_pRTech->GetPakLoadedInfo(handle);

	if (pakInfo && pakInfo->m_pszFileName)
	{
		Msg(eDLL_T::RTECH, "Unloading pak file: '%s'\n", pakInfo->m_pszFileName);

		if (strcmp(pakInfo->m_pszFileName, "mp_lobby.rpak") == 0)
			s_bBasePaksInitialized = false;
	}

	v_Pak_UnloadPak(handle);
}

void V_RTechGame::Attach() const
{
	DetourAttach(&v_Pak_LoadAsync, &Pak_LoadAsync);
	DetourAttach(&v_Pak_UnloadPak, &Pak_UnloadPak);
}

void V_RTechGame::Detach() const
{
	DetourDetach(&v_Pak_LoadAsync, &Pak_LoadAsync);
	DetourDetach(&v_Pak_UnloadPak, &Pak_UnloadPak);
}

// Symbols taken from R2 dll's.
PakLoadFuncs_t* g_pakLoadApi = nullptr;
