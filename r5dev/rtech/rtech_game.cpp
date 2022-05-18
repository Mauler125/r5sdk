//=============================================================================//
//
// Purpose: RTech game utilities
//
//=============================================================================//
#include "core/stdafx.h"
#include "engine/host_cmd.h"
#include "engine/sys_utils.h"
#include "engine/host_state.h"
#include "engine/cmodel_bsp.h"
#include "rtech/rtech_game.h"
#include "rtech/rtech_utils.h"

vector<RPakHandle_t> g_LoadedPakHandle{ };

//-----------------------------------------------------------------------------
// Purpose: load user-requested pak files on-demand
// Input  : *szPakFileName - 
//			*pMalloc - 
//			nIdx - 
//			bUnk - 
// Output : pak file handle on success, -1 (INVALID_PAK_HANDLE) on failure
//-----------------------------------------------------------------------------
RPakHandle_t CPakFile::AsyncLoad(const char* szPakFileName, uintptr_t pMalloc, int nIdx, bool bUnk)
{
	RPakHandle_t pakHandle = -1;
#ifdef DEDICATED
	// Extraneous files (useless on the dedicated server).
	if (strcmp(szPakFileName, "ui.rpak") == 0)
	{
		static const char* szReplacement = "common_empty.rpak";
		// Returning -1 (invalid handle) triggers engine error, call is inline.
		// Replacing the ui.rpak file here with a stub to avoid having to patch.
		DevMsg(eDLL_T::RTECH, "Loading pak file: '%s' for '%s'\n", szReplacement, szPakFileName);
		return pakHandle = CPakFile_AsyncLoad(szReplacement, pMalloc, nIdx, bUnk);
	}
	else if (strstr(szPakFileName, "ui")
		|| strstr(szPakFileName, "loadscreen")
		|| strstr(szPakFileName, "subtitles"))
	{
		return pakHandle;
	}
#endif // DEDICATED

	string svPakFilePathMod = "paks\\Win32\\" + string(szPakFileName);
	string svPakFilePathBase = "paks\\Win64\\" + string(szPakFileName);

	if (FileExists(svPakFilePathMod.c_str()) || FileExists(svPakFilePathBase.c_str()))
	{
		DevMsg(eDLL_T::RTECH, "Loading pak file: '%s'\n", szPakFileName);
		pakHandle = CPakFile_AsyncLoad(szPakFileName, pMalloc, nIdx, bUnk);

		if (pakHandle == -1)
		{
			Error(eDLL_T::RTECH, "%s: Failed read '%s' results '%u'\n", __FUNCTION__, szPakFileName, pakHandle);
		}
	}
	else
	{
		Error(eDLL_T::RTECH, "%s: Failed. File '%s' doesn't exist\n", __FUNCTION__, szPakFileName);
	}

	return pakHandle;
}

//-----------------------------------------------------------------------------
// Purpose: unloads loaded pak files
// Input  : handle - 
//-----------------------------------------------------------------------------
void CPakFile::Unload(RPakHandle_t handle)
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

	CPakFile_Unload(handle);
}

void RTech_Game_Attach()
{
	DetourAttach((LPVOID*)&CPakFile_AsyncLoad, &CPakFile::AsyncLoad);
	DetourAttach((LPVOID*)&CPakFile_Unload, &CPakFile::Unload);
}

void RTech_Game_Detach()
{
	DetourDetach((LPVOID*)&CPakFile_AsyncLoad, &CPakFile::AsyncLoad);
	DetourDetach((LPVOID*)&CPakFile_Unload, &CPakFile::Unload);
}

// Symbols taken from R2 dll's.
CPakFile* g_pakLoadApi = new CPakFile();
