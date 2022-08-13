#include "core/stdafx.h"
#include "vpklib/packedstore.h"
#include "filesystem/filesystem.h"

//-----------------------------------------------------------------------------
// Singleton FileSystem
//-----------------------------------------------------------------------------
CFileSystem_Stdio* FileSystem()
{
	return (*g_pFullFileSystem);
}

///////////////////////////////////////////////////////////////////////////////
CFileSystem_Stdio** g_pFullFileSystem  = nullptr;
CFileSystem_Stdio* g_pFileSystem_Stdio = nullptr;