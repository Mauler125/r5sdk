//=============================================================================//
//
// Purpose: Command line utilities
//
//=============================================================================//

#include "tier0_pch.h"
#include "tier0/commandline.h"
#include "tier1/cvar.h"

//-----------------------------------------------------------------------------
// Instance singleton and expose interface to rest of code
//-----------------------------------------------------------------------------
CCommandLine* CommandLine(void)
{
	return g_pCmdLine;
}

///////////////////////////////////////////////////////////////////////////////
CCommandLine* g_pCmdLine = nullptr;