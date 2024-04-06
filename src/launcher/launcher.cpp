//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Defines the entry point for the application.
//
// $NoKeywords: $
//===========================================================================//
#include "core/stdafx.h"
#include "core/logdef.h"
#include "core/init.h"
#include "tier0/crashhandler.h"
#include "tier0/commandline.h"
#include "tier1/strtools.h"
#include "launcher/launcher.h"
#include <eiface.h>

// Remove all but the last -game parameter.
// This is for mods based off something other than Half-Life 2 (like HL2MP mods).
// The Steam UI does 'steam -applaunch 320 -game c:\steam\steamapps\sourcemods\modname', but applaunch inserts
// its own -game parameter, which would supersede the one we really want if we didn't intercede here.
void RemoveSpuriousGameParameters()
{
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

LONG WINAPI TopLevelExceptionFilter(EXCEPTION_POINTERS* pExceptionPointers)
{
	// Don't run the unhandled exception filter from the
	// game if we have a valid vectored exception filter.
	if (g_CrashHandler.IsValid())
	{
		return NULL;
	}

	return v_TopLevelExceptionFilter(pExceptionPointers);
}

void VLauncher::Detour(const bool bAttach) const
{
	DetourSetup(&v_TopLevelExceptionFilter, &TopLevelExceptionFilter, bAttach);
	DetourSetup(&v_RemoveSpuriousGameParameters, &RemoveSpuriousGameParameters, bAttach);
}
