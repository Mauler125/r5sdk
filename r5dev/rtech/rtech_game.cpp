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

vector<RPakHandle_t> g_vLoadedPakHandle;

//-----------------------------------------------------------------------------
// Purpose: load user-requested pak files on-demand
// Input  : *szPakFileName - 
//			*pMalloc - 
//			nIdx - 
//			bUnk - 
// Output : pak file handle on success, INVALID_PAK_HANDLE on failure
//-----------------------------------------------------------------------------
RPakHandle_t CPakFile::LoadAsync(const char* szPakFileName, void* pMalloc, int nIdx, bool bUnk)
{
	RPakHandle_t pakHandle = INVALID_PAK_HANDLE;
	string svPakFileModPath = "paks\\Win32\\" + string(szPakFileName);
	string svPakFilePathBase = "paks\\Win64\\" + string(szPakFileName);

	if (FileExists(svPakFileModPath) || FileExists(svPakFilePathBase))
	{
		DevMsg(eDLL_T::RTECH, "Loading pak file: '%s'\n", szPakFileName);
		pakHandle = CPakFile_LoadAsync(szPakFileName, pMalloc, nIdx, bUnk);

		if (pakHandle == INVALID_PAK_HANDLE)
		{
			Error(eDLL_T::RTECH, NO_ERROR, "%s: Failed read '%s' results '%u'\n", __FUNCTION__, szPakFileName, pakHandle);
		}
	}
	else
	{
		Error(eDLL_T::RTECH, NO_ERROR, "%s: Failed. File '%s' doesn't exist\n", __FUNCTION__, szPakFileName);
	}

	return pakHandle;
}

//-----------------------------------------------------------------------------
// Purpose: unloads loaded pak files
// Input  : handle - 
//-----------------------------------------------------------------------------
void CPakFile::UnloadPak(RPakHandle_t handle)
{
	RPakLoadedInfo_t* pakInfo = g_pRTech->GetPakLoadedInfo(handle);

	if (pakInfo)
	{
		if (pakInfo->m_pszFileName)
		{
			DevMsg(eDLL_T::RTECH, "Unloading pak file: '%s'\n", pakInfo->m_pszFileName);

			if (strcmp(pakInfo->m_pszFileName, "mp_lobby.rpak") == 0)
				s_bBasePaksInitialized = false;
		}
	}

	CPakFile_UnloadPak(handle);
}

void V_RTechGame::Attach() const
{
	DetourAttach(&CPakFile_LoadAsync, &CPakFile::LoadAsync);
	DetourAttach(&CPakFile_UnloadPak, &CPakFile::UnloadPak);
}

void V_RTechGame::Detach() const
{
	DetourDetach(&CPakFile_LoadAsync, &CPakFile::LoadAsync);
	DetourDetach(&CPakFile_UnloadPak, &CPakFile::UnloadPak);
}

// Symbols taken from R2 dll's.
CPakFile* g_pakLoadApi = new CPakFile();
