#include "core/stdafx.h"
#include "engine/host_cmd.h"
#include "engine/sys_utils.h"
#include "rtech/rtech_game.h"

//-----------------------------------------------------------------------------
// Purpose: unloads asset files from the memory pool
//-----------------------------------------------------------------------------
void HRTech_UnloadAsset(std::int64_t a1, std::int64_t a2) // This ain't related to RTech, its a CSTDMem function.
{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	std::int64_t pAsset = a1;
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	std::int64_t pAsset = a2;
#endif
	// Return early if address is out of scope.
	if (pAsset <= 0x0000000000 || pAsset >= 0xFFFFFFFFFF)
	{
		return;
	}
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	return RTech_UnloadAsset(a1);
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	return RTech_UnloadAsset(a1, a2);
#endif
}

//-----------------------------------------------------------------------------
// Purpose: load user-requested pak files on-demand
//-----------------------------------------------------------------------------
void HRtech_AsyncLoad(std::string svPakFileName)
{
	std::string svPakFilePathMod = "paks\\Win32\\" + svPakFileName;
	std::string svPakFilePathBase = "paks\\Win64\\" + svPakFileName;

	if (FileExists(svPakFilePathMod.c_str()) || FileExists(svPakFilePathBase.c_str()))
	{
		int nPakId = RTech_AsyncLoad((void*)svPakFileName.c_str(), g_pMallocPool.GetPtr(), NULL, NULL);

		if (nPakId == 0xFFFFFFFF)
		{
			DevMsg(eDLL_T::RTECH, "RTech AsyncLoad failed read '%s' results '%u'\n", svPakFileName.c_str(), nPakId);
		}
	}
	else
	{
		DevMsg(eDLL_T::RTECH, "RTech AsyncLoad failed. File '%s' doesn't exist\n", svPakFileName.c_str());
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void* HJT_HelpWithAnything(bool bShouldLoadPak)
{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	static void* retaddr = reinterpret_cast<void*>(p_Host_NewGame.Offset(0x400).FindPatternSelf("48 8B ?? ?? ?? ?? 01", ADDRESS::Direction::DOWN).GetPtr());
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	static void* retaddr = reinterpret_cast<void*>(p_Host_NewGame.Offset(0x4A0).FindPatternSelf("48 8B ?? ?? ?? ?? 01", ADDRESS::Direction::DOWN).GetPtr());
#endif
	void* results = JT_HelpWithAnything(bShouldLoadPak);

	if (retaddr != _ReturnAddress()) // Check if this is called after 'PakFile_Init()'.
	{
		return results;
	}
	// Do stuff here after 'PakFile_Init()'.
	return results;
}

void RTech_Game_Attach()
{
	//DetourAttach((LPVOID*)&RTech_UnloadAsset, &HRTech_UnloadAsset);
	//DetourAttach((LPVOID*)&JT_HelpWithAnything, &HJT_HelpWithAnything);
}

void RTech_Game_Detach()
{
	//DetourAttach((LPVOID*)&RTech_UnloadAsset, &HRTech_UnloadAsset);
	//DetourAttach((LPVOID*)&JT_HelpWithAnything, &HJT_HelpWithAnything);
}
