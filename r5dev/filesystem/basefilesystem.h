#pragma once
#include "public/ifilesystem.h"

class CBaseFileSystem : public IFileSystem
{
public:
	//--------------------------------------------------------
	// Purpose: Static methods used for hooking.
	//--------------------------------------------------------
	static void Warning(CBaseFileSystem* pFileSystem, FileWarningLevel_t level, const char* fmt, ...);
	static bool VCheckDisk(const char* pszFilePath);
	static FileHandle_t VReadFromVPK(CBaseFileSystem* pFileSystem, FileHandle_t pResults, const char* pszFilePath);
	static bool VReadFromCache(CBaseFileSystem* pFileSystem, const char* pszFilePath, FileSystemCache* pCache);
	static void VAddMapPackFile(CBaseFileSystem* pFileSystem, const char* pPath, const char* pPathID, SearchPathAdd_t addType);
	static VPKData_t* VMountVPKFile(CBaseFileSystem* pFileSystem, const char* pszVpkPath);
	static const char* VUnmountVPKFile(CBaseFileSystem* pFileSystem, const char* pszVpkPath);

	CUtlString ReadString(FileHandle_t pFile);

protected:
	//----------------------------------------------------------------------------
	// Purpose: Functions implementing basic file system behavior.
	//----------------------------------------------------------------------------
	virtual FILE* FS_fopen(const char* filename, const char* options, unsigned flags, int64* size) = 0;
	virtual void FS_setbufsize(FILE* fp, unsigned nBytes) = 0;
	virtual void FS_fclose(FILE* fp) = 0;
	virtual void FS_fseek(FILE* fp, int64 pos, int seekType) = 0;
	virtual long FS_ftell(FILE* fp) = 0;
	virtual int FS_feof(FILE* fp) = 0;
	virtual size_t FS_fread(void* dest, size_t destSize, size_t size, FILE* fp) = 0;
	virtual size_t FS_fwrite(const void* src, size_t size, FILE* fp) = 0;
	virtual bool FS_setmode(FILE* fp, FileMode_t mode) = 0;
	virtual size_t FS_vfprintf(FILE* fp, const char* fmt, va_list list) = 0;
	virtual int FS_ferror(FILE* fp) = 0;
	virtual int FS_fflush(FILE* fp) = 0;
	virtual char* FS_fgets(char* dest, int destSize, FILE* fp) = 0;
	virtual int FS_stat(const char* path, struct _stat* buf, bool* pbLoadedFromSteamCache = NULL) = 0;
	virtual int FS_chmod(const char* path, int pmode) = 0;
	virtual HANDLE FS_FindFirstFile(const char* findname, WIN32_FIND_DATA* dat) = 0;
	virtual bool FS_FindNextFile(HANDLE handle, WIN32_FIND_DATA* dat) = 0;
	virtual bool FS_FindClose(HANDLE handle) = 0;
	virtual int FS_GetSectorSize(FILE*) = 0;
};

/* ==== CBASEFILESYSTEM ================================================================================================================================================= */
inline void(*CBaseFileSystem__Warning)(CBaseFileSystem* pFileSystem, FileWarningLevel_t level, const char* fmt, ...);
inline FileHandle_t(*CBaseFileSystem__LoadFromVPK)(CBaseFileSystem* pFileSystem, FileHandle_t pResults, const char* pszAssetName);
inline bool(*CBaseFileSystem__LoadFromCache)(CBaseFileSystem* pFileSystem, const char* pszAssetName, FileSystemCache* pCache);
inline void(*CBaseFileSystem__AddMapPackFile)(CBaseFileSystem* pFileSystem, const char* pPath, const char* pPathID, SearchPathAdd_t addType);
inline VPKData_t*(*CBaseFileSystem__MountVPKFile)(CBaseFileSystem* pFileSystem, const char* pszVpkPath);
inline const char* (*CBaseFileSystem__UnmountVPKFile)(CBaseFileSystem* pFileSystem, const char* pszVpkPath);
inline int(*CBaseFileSystem__GetMountedVPKHandle)(CBaseFileSystem* pFileSystem, const char* pszVpkPath);

extern CBaseFileSystem* g_pFileSystem;

///////////////////////////////////////////////////////////////////////////////
class VBaseFileSystem : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CBaseFileSystem::Warning", CBaseFileSystem__Warning);
		LogFunAdr("CBaseFileSystem::LoadFromVPK", CBaseFileSystem__LoadFromVPK);
		LogFunAdr("CBaseFileSystem::LoadFromCache", CBaseFileSystem__LoadFromCache);
		LogFunAdr("CBaseFileSystem::AddMapPackFile", CBaseFileSystem__AddMapPackFile);
		LogFunAdr("CBaseFileSystem::MountVPKFile", CBaseFileSystem__MountVPKFile);
		LogFunAdr("CBaseFileSystem::UnmountVPKFile", CBaseFileSystem__UnmountVPKFile);
		LogFunAdr("CBaseFileSystem::GetMountedVPKHandle", CBaseFileSystem__GetMountedVPKHandle);
		LogVarAdr("g_pFileSystem", g_pFileSystem);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("4C 89 4C 24 20 C3 CC CC CC CC CC CC CC CC CC CC 48").GetPtr(CBaseFileSystem__Warning);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 57 48 81 EC ?? ?? ?? ?? 49 8B C0 4C 8D 8C 24 ?? ?? ?? ??").GetPtr(CBaseFileSystem__LoadFromVPK);
		g_GameDll.FindPatternSIMD("40 53 48 81 EC ?? ?? ?? ?? 80 3D ?? ?? ?? ?? ?? 49 8B D8").GetPtr(CBaseFileSystem__LoadFromCache);
		g_GameDll.FindPatternSIMD("4C 89 44 24 ?? 48 89 54 24 ?? 55 ?? 41 54 41 55 48 8D AC 24 ?? ?? ?? ??").GetPtr(CBaseFileSystem__AddMapPackFile);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 57 48 81 EC ?? ?? ?? ?? 48 8B F9 4C 8D 05 ?? ?? ?? ??").GetPtr(CBaseFileSystem__MountVPKFile);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 57 48 83 EC 20 48 8B DA 48 8B F9 48 8B CB 48 8D 15 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 85 C0").GetPtr(CBaseFileSystem__UnmountVPKFile);
		g_GameDll.FindPatternSIMD("48 89 74 24 ?? 57 48 81 EC ?? ?? ?? ?? 48 8B F9 4C 8D 05 ?? ?? ?? ??").GetPtr(CBaseFileSystem__GetMountedVPKHandle);
	}
	virtual void GetVar(void) const
	{
		g_pFileSystem = g_GameDll.FindPatternSIMD("48 8D 05 ?? ?? ?? ?? 48 89 05 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ?? 48 89 05 ?? ?? ?? ?? E9 ?? ?? ?? ??")
			.FindPattern("48 89", CMemory::Direction::DOWN, 512, 2).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CBaseFileSystem*>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
