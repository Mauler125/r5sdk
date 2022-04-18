//=============================================================================//
//
// Purpose: RTech game utilities
//
//=============================================================================//
#include "core/stdafx.h"
#include "engine/host_cmd.h"
#include "engine/sys_utils.h"
#include "rtech/rtech_game.h"

std::vector<RPakHandle_t> g_LoadedPakHandle{ };

//-----------------------------------------------------------------------------
// Purpose: unloads asset files from the memory pool
//-----------------------------------------------------------------------------
void HPakFile_UnloadAsset(int64_t a1, int64_t a2) // This ain't related to RTech, its a CSTDMem function.
{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	int64_t pAsset = a1;
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	int64_t pAsset = a2;
#endif
	// Return early if address is out of scope.
	if (pAsset <= 0x0000000000 || pAsset >= 0xFFFFFFFFFF)
	{
		return;
	}
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	return CPakFile_UnloadAsset(a1);
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	return CPakFile_UnloadAsset(a1, a2);
#endif
}

//-----------------------------------------------------------------------------
// Purpose: load user-requested pak files on-demand
//-----------------------------------------------------------------------------
void HPakFile_AsyncLoad(string svPakFileName)
{
	string svPakFilePathMod = "paks\\Win32\\" + svPakFileName;
	string svPakFilePathBase = "paks\\Win64\\" + svPakFileName;

	if (FileExists(svPakFilePathMod.c_str()) || FileExists(svPakFilePathBase.c_str()))
	{
		int nPakId = CPakFile_AsyncLoad((void*)svPakFileName.c_str(), g_pMallocPool.GetPtr(), NULL, NULL);

		if (nPakId == 0xFFFFFFFF)
		{
			Error(eDLL_T::RTECH, "RTech_AsyncLoad: Failed read '%s' results '%u'\n", svPakFileName.c_str(), nPakId);
		}
	}
	else
	{
		Error(eDLL_T::RTECH, "RTech_AsyncLoad: Failed. File '%s' doesn't exist\n", svPakFileName.c_str());
	}
}

void RTech_Game_Attach()
{
	//DetourAttach((LPVOID*)&RTech_UnloadAsset, &HRTech_UnloadAsset);
}

void RTech_Game_Detach()
{
	//DetourDetach((LPVOID*)&RTech_UnloadAsset, &HRTech_UnloadAsset);
}
