//=============================================================================//
//
// Purpose: Command line utilities
//
//=============================================================================//

#include "core/stdafx.h"
#include "tier0/cvar.h"
#include "tier0/commandline.h"

//-----------------------------------------------------------------------------
// Purpose: Create a command line from the passed in string
//  Note that if you pass in a @filename, then the routine will read settings
//  from a file instead of the command line
//-----------------------------------------------------------------------------
void CCommandLine::CreateCmdLine(const char* pszCommandline)
{
	using OriginalFn = void(__thiscall*)(CCommandLine*, const char*);
	(*reinterpret_cast<OriginalFn**>(this))[0](this, pszCommandline);
}

//-----------------------------------------------------------------------------
// purpose: creates a command line from the arguments passed in
//-----------------------------------------------------------------------------
void CCommandLine::CreateCmdLine(int argc, char** argv)
{
	using OriginalFn = void(__thiscall*)(CCommandLine*, int, char**);
	return (*reinterpret_cast<OriginalFn**>(this))[1](this, argc, argv);
}

//-----------------------------------------------------------------------------
// purpose: allocates a pool for the command line [seems unused]
//-----------------------------------------------------------------------------
void CCommandLine::CreatePool(void* pMem)
{
	using OriginalFn = void(__thiscall*)(CCommandLine*, void*);
	(*reinterpret_cast<OriginalFn**>(this))[2](this, pMem);
}

//-----------------------------------------------------------------------------
// Purpose: Return current command line
// Output : const char
//-----------------------------------------------------------------------------
const char* CCommandLine::GetCmdLine(void)
{
	using OriginalFn = const char* (__thiscall*)(CCommandLine*);
	return (*reinterpret_cast<OriginalFn**>(this))[3](this);
}

//-----------------------------------------------------------------------------
// Purpose: Search for the parameter in the current commandline
// Input  : *psz - 
//			**ppszValue - 
// Output : char
//-----------------------------------------------------------------------------
const char* CCommandLine::CheckParm(const char* psz, const char** ppszValue)
{
	using OriginalFn = const char* (__thiscall*)(CCommandLine*, const char*, const char**);
	return (*reinterpret_cast<OriginalFn**>(this))[4](this, psz, ppszValue);
}

//-----------------------------------------------------------------------------
// Purpose: Remove specified string ( and any args attached to it ) from command line
// Input  : *pszParm - 
//-----------------------------------------------------------------------------
void CCommandLine::RemoveParm(void)
{
	using OriginalFn = void(__thiscall*)(CCommandLine*);
	return (*reinterpret_cast<OriginalFn**>(this))[5](this);
}

//-----------------------------------------------------------------------------
// Purpose: Append parameter and argument values to command line
// Input  : *pszParm - 
//			*pszValues - 
//-----------------------------------------------------------------------------
void CCommandLine::AppendParm(const char* pszParm, const char* pszValues)
{
	using OriginalFn = void(__thiscall*)(CCommandLine*, const char*, const char*);
	return (*reinterpret_cast<OriginalFn**>(this))[6](this, pszParm, pszParm);
}

//-----------------------------------------------------------------------------
// purpose: returns the argument after the one specified, or the default if not found
//-----------------------------------------------------------------------------
const char* CCommandLine::ParmValue(const char* psz, const char* pDefaultVal)
{
	using OriginalFn = const char* (__thiscall*)(CCommandLine*, const char*, const char*);
	return (*reinterpret_cast<OriginalFn**>(this))[7](this, psz, pDefaultVal);
}

int CCommandLine::ParmValue(const char* psz, int nDefaultVal)
{
	using OriginalFn = int(__thiscall*)(CCommandLine*, const char*, int);
	return (*reinterpret_cast<OriginalFn**>(this))[8](this, psz, nDefaultVal);
}

float CCommandLine::ParmValue(const char* psz, float flDefaultVal)
{
	using OriginalFn = float(__thiscall*)(CCommandLine*, const char*, float);
	return (*reinterpret_cast<OriginalFn**>(this))[9](this, psz, flDefaultVal);
}

//-----------------------------------------------------------------------------
// purpose: returns individual command line arguments
//-----------------------------------------------------------------------------
int CCommandLine::ParmCount(void)
{
	using OriginalFn = int(__thiscall*)(CCommandLine*);
	return (*reinterpret_cast<OriginalFn**>(this))[10](this);
}

int CCommandLine::FindParm(const char* psz)
{
	using OriginalFn = int(__thiscall*)(CCommandLine*, const char*);
	return (*reinterpret_cast<OriginalFn**>(this))[11](this, psz);
}

const char* CCommandLine::GetParm(int nIndex)
{
	using OriginalFn = const char* (__thiscall*)(CCommandLine*, int);
	return (*reinterpret_cast<OriginalFn**>(this))[12](this, nIndex);
}

void CCommandLine::SetParm(int nIndex, char const* pParm)
{
	using OriginalFn = void(__thiscall*)(CCommandLine*, int, const char*);
	return (*reinterpret_cast<OriginalFn**>(this))[13](this, nIndex, pParm);
}

///////////////////////////////////////////////////////////////////////////////
CCommandLine* g_pCmdLine = reinterpret_cast<CCommandLine*>(p_CCVar_GetCommandLineValue.FindPatternSelf("48 8D 0D", ADDRESS::Direction::DOWN, 250).ResolveRelativeAddressSelf(0x3, 0x7).GetPtr());
