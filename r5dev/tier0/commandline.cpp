//=============================================================================//
//
// Purpose: Command line utilities
//
//=============================================================================//

#include "core/stdafx.h"
#include "tier0/commandline.h"
#include "tier1/cvar.h"

//-----------------------------------------------------------------------------
// Purpose: Create a command line from the passed in string
// Note that if you pass in a @filename, then the routine will read settings
// from a file instead of the command line
//-----------------------------------------------------------------------------
void CCommandLine::CreateCmdLine(const char* pszCommandline)
{
	static int index = 0;
	CallVFunc<void>(index, this, pszCommandline);
}

//-----------------------------------------------------------------------------
// Purpose: creates a command line from the arguments passed in
//-----------------------------------------------------------------------------
void CCommandLine::CreateCmdLine(int argc, char** argv)
{
	static int index = 1;
	CallVFunc<void>(index, this, argc, argv);
}

//-----------------------------------------------------------------------------
// Purpose: allocates a pool for the command line [seems unused]
//-----------------------------------------------------------------------------
void CCommandLine::CreatePool(void* pMem)
{
	static int index = 2;
	CallVFunc<void>(index, this, pMem);
}

//-----------------------------------------------------------------------------
// Purpose: Return current command line
// Output : const char
//-----------------------------------------------------------------------------
const char* CCommandLine::GetCmdLine(void)
{
	static int index = 3;
	return CallVFunc<const char*>(index, this);
}

//-----------------------------------------------------------------------------
// Purpose: Search for the parameter in the current commandline
// Input  : *psz - 
//			**ppszValue - 
// Output : char
//-----------------------------------------------------------------------------
const char* CCommandLine::CheckParm(const char* psz, const char** ppszValue)
{
	static int index = 4;
	return CallVFunc<const char*>(index, this, psz, ppszValue);
}

//-----------------------------------------------------------------------------
// Purpose: Remove specified string ( and any args attached to it ) from command line
// Input  : *pszParm - 
//-----------------------------------------------------------------------------
void CCommandLine::RemoveParm(const char* pszParm)
{
	static int index = 5;
	CallVFunc<void>(index, this, pszParm);
}

//-----------------------------------------------------------------------------
// Purpose: Append parameter and argument values to command line
// Input  : *pszParm - 
//			*pszValues - 
//-----------------------------------------------------------------------------
void CCommandLine::AppendParm(const char* pszParm, const char* pszValues)
{
	static int index = 6;
	CallVFunc<void>(index, this, pszParm, pszValues);
}

//-----------------------------------------------------------------------------
// Purpose: returns the argument after the one specified, or the default if not found
//-----------------------------------------------------------------------------
const char* CCommandLine::ParmValue(const char* psz, const char* pDefaultVal)
{
	static int index = 7;
	return CallVFunc<const char*>(index, this, psz, pDefaultVal);
}
int CCommandLine::ParmValue(const char* psz, int nDefaultVal)
{
	static int index = 8;
	return CallVFunc<int>(index, this, psz, nDefaultVal);
}
float CCommandLine::ParmValue(const char* psz, float flDefaultVal)
{
	static int index = 9;
	return CallVFunc<float>(index, this, psz, flDefaultVal);
}

//-----------------------------------------------------------------------------
// Purpose: returns individual command line arguments
//-----------------------------------------------------------------------------
int CCommandLine::ParmCount(void)
{
	static int index = 10;
	return CallVFunc<int>(index, this);
}

int CCommandLine::FindParm(const char* psz)
{
	static int index = 11;
	return CallVFunc<int>(index, this, psz);
}

const char* CCommandLine::GetParm(int nIndex)
{
	static int index = 12;
	return CallVFunc<const char*>(index, this, nIndex);
}

void CCommandLine::SetParm(int nIndex, char const* pParm)
{
	static int index = 14;
	CallVFunc<void>(index, this, nIndex, pParm);
}

///////////////////////////////////////////////////////////////////////////////
CCommandLine* g_pCmdLine = nullptr;

//-----------------------------------------------------------------------------
// Instance singleton and expose interface to rest of code
//-----------------------------------------------------------------------------
CCommandLine* CommandLine(void)
{
	return g_pCmdLine;
}
