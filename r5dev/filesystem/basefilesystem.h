#pragma once
#include "public/ifilesystem.h"

class CBaseFileSystem : public IFileSystem
{
public:
	//--------------------------------------------------------
	// Purpose: Static methods used for hooking.
	//--------------------------------------------------------
	static void Warning(CBaseFileSystem* pFileSystem, FileWarningLevel_t level, const char* fmt, ...);
	static FileHandle_t VReadFromVPK(CBaseFileSystem* pVpk, FileHandle_t pResults, char* pszFilePath);
	static bool VReadFromCache(CBaseFileSystem* pFileSystem, char* pszFilePath, void* pResults);

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
inline auto CBaseFileSystem_Warning = p_CBaseFileSystem_Warning.RCast<void(*)(CBaseFileSystem* thisptr, FileWarningLevel_t level, const char* fmt, ...)>();

inline CMemory p_CBaseFileSystem_LoadFromVPK;
inline auto CBaseFileSystem_VLoadFromVPK = p_CBaseFileSystem_LoadFromVPK.RCast<FileHandle_t(*)(CBaseFileSystem* thisptr, FileHandle_t pResults, char* pszAssetName)>();

inline CMemory p_CBaseFileSystem_LoadFromCache;
inline auto CBaseFileSystem_VLoadFromCache = p_CBaseFileSystem_LoadFromCache.RCast<bool(*)(CBaseFileSystem* thisptr, char* pszAssetName, void* pResults)>();

extern CBaseFileSystem* g_pFileSystem;

///////////////////////////////////////////////////////////////////////////////
void CBaseFileSystem_Attach();
void CBaseFileSystem_Detach();

///////////////////////////////////////////////////////////////////////////////
class VBaseFileSystem : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: CBaseFileSystem::Warning             : {:#18x} |\n", p_CBaseFileSystem_Warning.GetPtr());
		spdlog::debug("| FUN: CBaseFileSystem::LoadFromVPK         : {:#18x} |\n", p_CBaseFileSystem_LoadFromVPK.GetPtr());
		spdlog::debug("| FUN: CBaseFileSystem::LoadFromCache       : {:#18x} |\n", p_CBaseFileSystem_LoadFromCache.GetPtr());
		spdlog::debug("| VAR: g_pFileSystem                        : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_pFileSystem));
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		p_CBaseFileSystem_Warning          = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x4C\x89\x4C\x24\x20\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x48"), "xxxxxx??????????x");
		p_CBaseFileSystem_LoadFromVPK      = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x57\x48\x81\xEC\x00\x00\x00\x00\x49\x8B\xC0\x4C\x8D\x8C\x24\x00\x00\x00\x00"), "xxxx?xxxx????xxxxxxx????");
		p_CBaseFileSystem_LoadFromCache    = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x81\xEC\x00\x00\x00\x00\x80\x3D\x00\x00\x00\x00\x00\x49\x8B\xD8"), "xxxxx????xx?????xxx");

		CBaseFileSystem_Warning           = p_CBaseFileSystem_Warning.RCast<void(*)(CBaseFileSystem*, FileWarningLevel_t, const char*, ...)>();            /*4C 89 4C 24 20 C3 ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 48*/
		CBaseFileSystem_VLoadFromVPK      = p_CBaseFileSystem_LoadFromVPK.RCast<FileHandle_t(*)(CBaseFileSystem*, FileHandle_t, char*)>();                 /*48 89 5C 24 ?? 57 48 81 EC ?? ?? ?? ?? 49 8B C0 4C 8D 8C 24 ?? ?? ?? ??*/
		CBaseFileSystem_VLoadFromCache    = p_CBaseFileSystem_LoadFromCache.RCast<bool(*)(CBaseFileSystem*, char*, void*)>();                              /*40 53 48 81 EC ?? ?? ?? ?? 80 3D ?? ?? ?? ?? ?? 49 8B D8*/
	}
	virtual void GetVar(void) const
	{
		g_pFileSystem = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\xE9\x00\x00\x00\x00"),
			"xxx????xxx????xxx????xxx????xxx????x????").FindPattern("48 89", CMemory::Direction::DOWN, 512, 2).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CBaseFileSystem*>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VBaseFileSystem);
