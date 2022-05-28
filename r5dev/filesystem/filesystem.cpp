#include "core/stdafx.h"
#include "vpklib/packedstore.h"
#include "filesystem/filesystem.h"

//-----------------------------------------------------------------------------
// Purpose: create the search path.
// Input  : *pPath - 
//			*pPathID - 
//			addType - 
//-----------------------------------------------------------------------------
void IFileSystem::AddSearchPath(const char* pPath, const char* pPathID, SearchPathAdd_t addType)
{
	static int index = 12;
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
	static int index = 13;
	return CallVFunc<bool>(index, this, pPath, pPathID);
}

//-----------------------------------------------------------------------------
// Purpose: read file from cache.
// Input  : *pPath - 
//			*pResult - 
// Output : true if exists, false otherwise.
//-----------------------------------------------------------------------------
bool IFileSystem::ReadFromCache(const char* pPath, void* pResult)
{
	static int index = 76;
	return CallVFunc<bool>(index, this, pPath, pResult);
}

//-----------------------------------------------------------------------------
// Purpose: mount specified VPK file (to access data).
// Input  : *pPath - 
// Output : *VPKData_t (information about mounted VPK file)
//-----------------------------------------------------------------------------
VPKData_t* IFileSystem::MountVPK(const char* pPath)
{
	static int index = 92;
	return CallVFunc<VPKData_t*>(index, this, pPath);
}

///////////////////////////////////////////////////////////////////////////////
IFileSystem* g_pFullFileSystem = nullptr;
CFileSystem_Stdio* g_pFileSystem_Stdio = nullptr;