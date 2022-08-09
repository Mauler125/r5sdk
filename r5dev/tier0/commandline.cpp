//=============================================================================//
//
// Purpose: Command line utilities
//
//=============================================================================//

#include "core/stdafx.h"
#include "tier0/commandline.h"
#include "tier1/cvar.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCommandLine::CCommandLine(void)
{
	m_pszCmdLine = NULL;
	m_nParmCount = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCommandLine::~CCommandLine(void)
{
	CleanUpParms();
	delete[] m_pszCmdLine;
}

//-----------------------------------------------------------------------------
// Purpose: Create a command line from the passed in string
// Note that if you pass in a @filename, then the routine will read settings
// from a file instead of the command line
//-----------------------------------------------------------------------------
void CCommandLine::CreateCmdLine(const char* pszCommandline)
{
	const int index = 0;
	CallVFunc<void>(index, this, pszCommandline);
}

//-----------------------------------------------------------------------------
// Purpose: creates a command line from the arguments passed in
//-----------------------------------------------------------------------------
void CCommandLine::CreateCmdLine(int argc, char** argv)
{
	const int index = 1;
	CallVFunc<void>(index, this, argc, argv);
}

//-----------------------------------------------------------------------------
// Purpose: allocates a pool for the command line [seems unused]
//-----------------------------------------------------------------------------
void CCommandLine::CreatePool(void* pMem)
{
	const int index = 2;
	CallVFunc<void>(index, this, pMem);
}

//-----------------------------------------------------------------------------
// Purpose: Return current command line
// Output : const char
//-----------------------------------------------------------------------------
const char* CCommandLine::GetCmdLine(void)
{
	const int index = 3;
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
	const int index = 4;
	return CallVFunc<const char*>(index, this, psz, ppszValue);
}

//-----------------------------------------------------------------------------
// Purpose: Remove specified string ( and any args attached to it ) from command line
// Input  : *pszParm - 
//-----------------------------------------------------------------------------
void CCommandLine::RemoveParm(const char* pszParm)
{
	const int index = 5;
	CallVFunc<void>(index, this, pszParm);
}

//-----------------------------------------------------------------------------
// Purpose: Append parameter and argument values to command line
// Input  : *pszParm - 
//			*pszValues - 
//-----------------------------------------------------------------------------
void CCommandLine::AppendParm(const char* pszParm, const char* pszValues)
{
	const int index = 6;
	CallVFunc<void>(index, this, pszParm, pszValues);
}

//-----------------------------------------------------------------------------
// Purpose: returns the argument after the one specified, or the default if not found
//-----------------------------------------------------------------------------
float CCommandLine::ParmValue(const char* psz, float flDefaultVal)
{
	const int index = 7;
	return CallVFunc<float>(index, this, psz, flDefaultVal);
}
int CCommandLine::ParmValue(const char* psz, int nDefaultVal)
{
	const int index = 8;
	return CallVFunc<int>(index, this, psz, nDefaultVal);
}
const char* CCommandLine::ParmValue(const char* psz, const char* pDefaultVal)
{
	const int index = 9;
	return CallVFunc<const char*>(index, this, psz, pDefaultVal);
}

//-----------------------------------------------------------------------------
// Purpose: returns individual command line arguments
//-----------------------------------------------------------------------------
int CCommandLine::ParmCount(void)
{
	const int index = 10;
	return CallVFunc<int>(index, this);
}

int CCommandLine::FindParm(const char* psz)
{
	const int index = 11;
	return CallVFunc<int>(index, this, psz);
}

const char* CCommandLine::GetParm(int nIndex)
{
	const int index = 12;
	return CallVFunc<const char*>(index, this, nIndex);
}

void CCommandLine::SetParm(int nIndex, char const* pParm)
{
	const int index = 14;
	CallVFunc<void>(index, this, nIndex, pParm);
}

//-----------------------------------------------------------------------------
// Individual command line arguments
//-----------------------------------------------------------------------------
void CCommandLine::CleanUpParms(void)
{
	for (int i = 0; i < m_nParmCount; ++i)
	{
		delete[] m_ppParms[i];
		m_ppParms[i] = NULL;
	}
	m_nParmCount = 0;
}

//-----------------------------------------------------------------------------
// Instance singleton and expose interface to rest of code
//-----------------------------------------------------------------------------
CCommandLine* CommandLine(void)
{
	return g_pCmdLine;
}

///////////////////////////////////////////////////////////////////////////////
CCommandLine* g_pCmdLine = nullptr;