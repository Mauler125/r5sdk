//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Defines the entry point for the application.
//
// $NoKeywords: $
//===========================================================================//
#include "core/stdafx.h"
#include "core/logdef.h"
#include "tier0/crashhandler.h"
#include "tier0/commandline.h"
#include "tier1/strtools.h"
#include "launcher/launcher.h"
#include <eiface.h>

int LauncherMain(HINSTANCE hInstance)
{
	// Flush buffers every 5 seconds for every logger.
	// Has to be done here, don't move this to SpdLog
	// init, as this could cause deadlocks on certain
	// compilers (VS2017)!!!
	spdlog::flush_every(std::chrono::seconds(5));

	int results = v_LauncherMain(hInstance);
	DevMsg(eDLL_T::NONE, "%s returned: %s\n", __FUNCTION__, ExitCodeToString(results));
	return results;
}

#if !defined (GAMEDLL_S0) || !defined (GAMEDLL_S1)
// Remove all but the last -game parameter.
// This is for mods based off something other than Half-Life 2 (like HL2MP mods).
// The Steam UI does 'steam -applaunch 320 -game c:\steam\steamapps\sourcemods\modname', but applaunch inserts
// its own -game parameter, which would supersede the one we really want if we didn't intercede here.
void RemoveSpuriousGameParameters()
{
	AppendSDKParametersPreInit();

	// Find the last -game parameter.
	int nGameArgs = 0;
	char lastGameArg[MAX_PATH];
	for (int i = 0; i < CommandLine()->ParmCount() - 1; i++)
	{
		if (Q_stricmp(CommandLine()->GetParm(i), "-game") == 0)
		{
			Q_snprintf(lastGameArg, sizeof(lastGameArg), "\"%s\"", CommandLine()->GetParm(i + 1));
			++nGameArgs;
			++i;
		}
	}

	// We only care if > 1 was specified.
	if (nGameArgs > 1)
	{
		CommandLine()->RemoveParm("-game");
		CommandLine()->AppendParm("-game", lastGameArg);
	}
}
#endif

// Append required command line parameters.
// This avoids having all these in the startup configuration files
// as all there are required to run the game with the game sdk.
void AppendSDKParametersPreInit()
{
	const bool bDedicated = IsDedicated();
	if (bDedicated)
	{
		CommandLine()->AppendParm("-collate", "");
		CommandLine()->AppendParm("-multiple", "");
		CommandLine()->AppendParm("-noorigin", "");
		CommandLine()->AppendParm("-nodiscord", "");
		CommandLine()->AppendParm("-noshaderapi", "");
		CommandLine()->AppendParm("-nobakedparticles", "");
		CommandLine()->AppendParm("-novid", "");
		CommandLine()->AppendParm("-nomenuvid", "");
		CommandLine()->AppendParm("-nosound", "");
		CommandLine()->AppendParm("-nomouse", "");
		CommandLine()->AppendParm("-nojoy", "");
		CommandLine()->AppendParm("-nosendtable", "");
	}
}

const char* ExitCodeToString(int nCode)
{
	switch (nCode)
	{
	case EXIT_SUCCESS:
		return "EXIT_SUCCESS";
	case EXIT_FAILURE:
		return "EXIT_FAILURE";
	default:
		return "UNKNOWN_EXIT_CODE";
	}
}

LONG WINAPI TopLevelExceptionFilter(EXCEPTION_POINTERS* pExceptionPointers)
{
	// Don't run the unhandled exception filter from the
	// game if we have a valid vectored exception filter.
	if (g_CrashHandler)
	{
		return NULL;
	}

	return v_TopLevelExceptionFilter(pExceptionPointers);
}

void VLauncher::Attach(void) const
{
	DetourAttach((LPVOID*)&v_LauncherMain, &LauncherMain);
	DetourAttach((LPVOID*)&v_TopLevelExceptionFilter, &TopLevelExceptionFilter);
#if !defined (GAMEDLL_S0) && !defined (GAMEDLL_S1)
	DetourAttach((LPVOID*)&v_RemoveSpuriousGameParameters, &RemoveSpuriousGameParameters);
#endif
}
void VLauncher::Detach(void) const
{
	DetourDetach((LPVOID*)&v_LauncherMain, &LauncherMain);
	DetourDetach((LPVOID*)&v_TopLevelExceptionFilter, &TopLevelExceptionFilter);
#if !defined (GAMEDLL_S0) && !defined (GAMEDLL_S1)
	DetourDetach((LPVOID*)&v_RemoveSpuriousGameParameters, &RemoveSpuriousGameParameters);
#endif
}