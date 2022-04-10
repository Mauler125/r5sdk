#include "core/stdafx.h"
#include "vpklib/packedstore.h"
#include "filesystem/filesystem.h"

//-----------------------------------------------------------------------------
// Purpose: create the search path.
// Input  : *pPath - 
//			*pPathID - 
//			addType - 
//-----------------------------------------------------------------------------
void CFileSystem_Stdio::AddSearchPath(const char* pPath, const char* pPathID, SearchPathAdd_t addType)
{
	static int index = 12;
	CallVFunc<void>(index, this, pPath, pPathID, addType);
}

//-----------------------------------------------------------------------------
// Purpose: read file from cache.
// Input  : *pPath - 
//			*pResult - 
// Output : true if exists, false otherwise.
//-----------------------------------------------------------------------------
bool CFileSystem_Stdio::ReadFromCache(const char* pPath, void* pResult)
{
	static int index = 76;
	return CallVFunc<bool>(index, this, pPath, pResult);
}

//-----------------------------------------------------------------------------
// Purpose: mount specified VPK file (to access data).
// Input  : *pPath - 
// Output : *VPKData_t (information about mounted VPK file)
//-----------------------------------------------------------------------------
VPKData_t* CFileSystem_Stdio::MountVPK(const char* pPath)
{
	static int index = 92;
	return CallVFunc<VPKData_t*>(index, this, pPath);
}

///////////////////////////////////////////////////////////////////////////////
CFileSystem_Stdio* g_pFileSystem_Stdio = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x20\x48\x8B\xD9\xFF\x15\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00"), 
	"xxxxxxxxxxx????xxx????").FindPatternSelf("48 8D", CMemory::Direction::DOWN).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CFileSystem_Stdio*>();