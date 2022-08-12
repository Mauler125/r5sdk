#include "core/stdafx.h"
#include "vpklib/packedstore.h"
#include "filesystem/filesystem.h"

//-----------------------------------------------------------------------------
// Singleton FileSystem
//-----------------------------------------------------------------------------
CFileSystem_Stdio* FileSystem()
{
	return *g_pFullFileSystem;
}

//-----------------------------------------------------------------------------
// Purpose: create the search path.
// Input  : *pPath - 
//			*pPathID - 
//			addType - 
//-----------------------------------------------------------------------------
void IFileSystem::AddSearchPath(const char* pPath, const char* pPathID, SearchPathAdd_t addType)
{
	const int index = (12 - FS_VFTABLE_SHIFT);
	CallVFunc<void>(index, this, pPath, pPathID, addType);
}

//-----------------------------------------------------------------------------
// Purpose: removes the search path.
// Input  : *pPath - 
//			*pPathID - 
//			addType - 
// Output : true on success, false otherwise.
//-----------------------------------------------------------------------------
bool IFileSystem::RemoveSearchPath(const char* pPath, const char* pPathID)
{
	const int index = (13 - FS_VFTABLE_SHIFT);
	return CallVFunc<bool>(index, this, pPath, pPathID);
}

//-----------------------------------------------------------------------------
// Purpose: print to file.
// Input  : file - 
//			*pFormat - 
//			... - 
// Output : number of bytes written.
//-----------------------------------------------------------------------------
int IFileSystem::FPrintf(FileHandle_t file, const char* pFormat, ...) FMTFUNCTION(3, 4)
{
	const int index = (29 - FS_VFTABLE_SHIFT);
	char buf[65560];
	{/////////////////////////////
		va_list args{};
		va_start(args, pFormat);

		vsnprintf(buf, sizeof(buf), pFormat, args);

		buf[sizeof(buf) - 1] = '\0';
		va_end(args);
	}/////////////////////////////

	return CallVFunc<int>(index, this, file, buf);
}

//-----------------------------------------------------------------------------
// Purpose: read file from cache.
// Input  : *pPath - 
//			*pResult - 
// Output : true if exists, false otherwise.
//-----------------------------------------------------------------------------
bool IFileSystem::ReadFromCache(const char* pPath, void* pResult)
{
	const int index = (76 - FS_VFTABLE_SHIFT);
	return CallVFunc<bool>(index, this, pPath, pResult);
}

//-----------------------------------------------------------------------------
// Purpose: mount specified VPK file (to access data).
// Input  : *pPath - 
// Output : *VPKData_t (information about mounted VPK file)
//-----------------------------------------------------------------------------
VPKData_t* IFileSystem::MountVPK(const char* pPath)
{
	const int index = (92 - FS_VFTABLE_SHIFT);
	return CallVFunc<VPKData_t*>(index, this, pPath);
}

///////////////////////////////////////////////////////////////////////////////
CFileSystem_Stdio** g_pFullFileSystem  = nullptr;
CFileSystem_Stdio* g_pFileSystem_Stdio = nullptr;