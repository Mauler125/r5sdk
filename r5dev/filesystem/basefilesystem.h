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
inline CMemory p_CBaseFileSystem_Warning;
inline void(*v_CBaseFileSystem_Warning)(CBaseFileSystem* pFileSystem, FileWarningLevel_t level, const char* fmt, ...);

inline CMemory p_CBaseFileSystem_LoadFromVPK;
inline FileHandle_t(*v_CBaseFileSystem_LoadFromVPK)(CBaseFileSystem* pFileSystem, FileHandle_t pResults, const char* pszAssetName);

inline CMemory p_CBaseFileSystem_LoadFromCache;
inline bool(*v_CBaseFileSystem_LoadFromCache)(CBaseFileSystem* pFileSystem, const char* pszAssetName, FileSystemCache* pCache);

inline CMemory p_CBaseFileSystem_AddMapPackFile;
inline void(*v_CBaseFileSystem_AddMapPackFile)(CBaseFileSystem* pFileSystem, const char* pPath, const char* pPathID, SearchPathAdd_t addType);

inline CMemory p_CBaseFileSystem_MountVPKFile;
inline VPKData_t*(*v_CBaseFileSystem_MountVPKFile)(CBaseFileSystem* pFileSystem, const char* pszVpkPath);

inline CMemory p_CBaseFileSystem_UnmountVPKFile;
inline const char* (*v_CBaseFileSystem_UnmountVPKFile)(CBaseFileSystem* pFileSystem, const char* pszVpkPath);

inline CMemory p_CBaseFileSystem_GetMountedVPKHandle;
inline int(*v_CBaseFileSystem_GetMountedVPKHandle)(CBaseFileSystem* pFileSystem, const char* pszVpkPath);

extern CBaseFileSystem* g_pFileSystem;

///////////////////////////////////////////////////////////////////////////////
class VBaseFileSystem : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CBaseFileSystem::Warning", p_CBaseFileSystem_Warning.GetPtr());
		LogFunAdr("CBaseFileSystem::LoadFromVPK", p_CBaseFileSystem_LoadFromVPK.GetPtr());
		LogFunAdr("CBaseFileSystem::LoadFromCache", p_CBaseFileSystem_LoadFromCache.GetPtr());
		LogFunAdr("CBaseFileSystem::AddMapPackFile", p_CBaseFileSystem_AddMapPackFile.GetPtr());
		LogFunAdr("CBaseFileSystem::MountVPKFile", p_CBaseFileSystem_MountVPKFile.GetPtr());
		LogFunAdr("CBaseFileSystem::UnmountVPKFile", p_CBaseFileSystem_UnmountVPKFile.GetPtr());
		LogFunAdr("CBaseFileSystem::GetMountedVPKHandle", p_CBaseFileSystem_GetMountedVPKHandle.GetPtr());
		LogVarAdr("g_pFileSystem", reinterpret_cast<uintptr_t>(g_pFileSystem));
	}
	virtual void GetFun(void) const
	{
		p_CBaseFileSystem_Warning             = g_GameDll.FindPatternSIMD("4C 89 4C 24 20 C3 CC CC CC CC CC CC CC CC CC CC 48");
		p_CBaseFileSystem_LoadFromVPK         = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 57 48 81 EC ?? ?? ?? ?? 49 8B C0 4C 8D 8C 24 ?? ?? ?? ??");
		p_CBaseFileSystem_LoadFromCache       = g_GameDll.FindPatternSIMD("40 53 48 81 EC ?? ?? ?? ?? 80 3D ?? ?? ?? ?? ?? 49 8B D8");
		p_CBaseFileSystem_AddMapPackFile      = g_GameDll.FindPatternSIMD("4C 89 44 24 ?? 48 89 54 24 ?? 55 ?? 41 54 41 55 48 8D AC 24 ?? ?? ?? ??");
		p_CBaseFileSystem_MountVPKFile        = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 57 48 81 EC ?? ?? ?? ?? 48 8B F9 4C 8D 05 ?? ?? ?? ??");
		p_CBaseFileSystem_UnmountVPKFile      = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 57 48 83 EC 20 48 8B DA 48 8B F9 48 8B CB 48 8D 15 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 85 C0");
		p_CBaseFileSystem_GetMountedVPKHandle = g_GameDll.FindPatternSIMD("48 89 74 24 ?? 57 48 81 EC ?? ?? ?? ?? 48 8B F9 4C 8D 05 ?? ?? ?? ??");

		v_CBaseFileSystem_Warning             = p_CBaseFileSystem_Warning.RCast<void(*)(CBaseFileSystem*, FileWarningLevel_t, const char*, ...)>();  /*4C 89 4C 24 20 C3 ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 48*/
		v_CBaseFileSystem_LoadFromVPK         = p_CBaseFileSystem_LoadFromVPK.RCast<FileHandle_t(*)(CBaseFileSystem*, FileHandle_t, const char*)>(); /*48 89 5C 24 ?? 57 48 81 EC ?? ?? ?? ?? 49 8B C0 4C 8D 8C 24 ?? ?? ?? ??*/
		v_CBaseFileSystem_LoadFromCache       = p_CBaseFileSystem_LoadFromCache.RCast<bool(*)(CBaseFileSystem*, const char*, FileSystemCache*)>();   /*40 53 48 81 EC ?? ?? ?? ?? 80 3D ?? ?? ?? ?? ?? 49 8B D8*/
		v_CBaseFileSystem_AddMapPackFile      = p_CBaseFileSystem_AddMapPackFile.RCast<void (*)(CBaseFileSystem*, const char*, const char*, SearchPathAdd_t)>();
		v_CBaseFileSystem_MountVPKFile        = p_CBaseFileSystem_MountVPKFile.RCast<VPKData_t*(*)(CBaseFileSystem*, const char*)>();                /*48 89 5C 24 ?? 48 89 6C 24 ?? 57 48 81 EC ?? ?? ?? ?? 48 8B F9 4C 8D 05 ?? ?? ?? ??*/
		v_CBaseFileSystem_UnmountVPKFile      = p_CBaseFileSystem_UnmountVPKFile.RCast<const char*(*)(CBaseFileSystem*, const char*)>();             /*48 89 5C 24 ?? 57 48 83 EC 20 48 8B DA 48 8B F9 48 8B CB 48 8D 15 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 85 C0*/
		v_CBaseFileSystem_GetMountedVPKHandle = p_CBaseFileSystem_GetMountedVPKHandle.RCast<int (*)(CBaseFileSystem*, const char*)>();               /*48 89 74 24 ?? 57 48 81 EC ?? ?? ?? ?? 48 8B F9 4C 8D 05 ?? ?? ?? ??*/
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
