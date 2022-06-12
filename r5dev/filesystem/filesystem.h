#ifndef FILESYSTEM_H
#define FILESYSTEM_H
#include "vpklib/packedstore.h"
#include "filesystem/basefilesystem.h"

#define GAMEINFOPATH_TOKEN		"|gameinfo_path|"
#define BASESOURCEPATHS_TOKEN	"|all_source_engine_paths|"

class IFileSystem
{
public:
	void AddSearchPath(const char* pPath, const char* pPathID, SearchPathAdd_t addType);
	bool RemoveSearchPath(const char* pPath, const char* pPathID);
	bool ReadFromCache(const char* pPath, void* pResult);
	VPKData_t* MountVPK(const char* pVpkPath);
};

class CFileSystem_Stdio : public IFileSystem, public CBaseFileSystem
{
};

extern IFileSystem* g_pFullFileSystem; // Ptr to g_pFileSystem_Stdio.
extern CFileSystem_Stdio* g_pFileSystem_Stdio;

///////////////////////////////////////////////////////////////////////////////
class VFileSystem_Stdio : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| VAR: g_pFullFileSystem                    : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_pFullFileSystem));
		spdlog::debug("| VAR: g_pFileSystem_Stdio                  : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_pFileSystem_Stdio));
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const
	{
		g_pFullFileSystem = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\x0D\x00\x00\x00\x00\x45\x33\xC0\x48\x83\xC1\x08\x48\x8B\x01"),
			"xxx????xxxxxxxxxx").ResolveRelativeAddressSelf(0x3, 0x7).RCast<IFileSystem*>();

		g_pFileSystem_Stdio = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\xE9\x00\x00\x00\x00"),
			"xxx????xxx????xxx????xxx????xxx????x????").FindPattern("48 89", CMemory::Direction::DOWN, 512, 1).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CFileSystem_Stdio*>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VFileSystem_Stdio);
#endif // !FILESYSTEM_H
