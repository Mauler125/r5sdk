#include "tier1/keyvalues_iface.h"
#include "filesystem/filesystem.h"

//-----------------------------------------------------------------------------
// Purpose: reads a keyvalues file
// Input  : *pFileSystem - 
//			* pFileName - 
// Output : pointer to KeyValues object
//-----------------------------------------------------------------------------
static KeyValues* ReadKeyValuesFile(CFileSystem_Stdio* pFileSystem, const char* pFileName)
{
	return KeyValues__ReadKeyValuesFile(pFileSystem, pFileName);
}

///////////////////////////////////////////////////////////////////////////////
void VKeyValues::Detour(const bool bAttach) const
{
	DetourSetup(&KeyValues__ReadKeyValuesFile, &ReadKeyValuesFile, bAttach);
}

///////////////////////////////////////////////////////////////////////////////
CThreadMutex g_InstalledMapsMutex;