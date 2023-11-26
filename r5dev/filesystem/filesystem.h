#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "public/ifilesystem.h"
#include "public/ifile.h"
#include "filesystem/basefilesystem.h"

class CFileSystem_Stdio : public CBaseFileSystem
{
protected:
	// implementation of CBaseFileSystem virtual functions
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
	virtual char* FS_fgets(char* dest, unsigned int destSize) = 0;
	virtual int FS_stat(const char* path, struct _stat* buf, bool* pbLoadedFromSteamCache = NULL) = 0;
	virtual int FS_chmod(const char* path, int pmode) = 0;
	virtual HANDLE FS_FindFirstFile(const char* findname, WIN32_FIND_DATA* dat) = 0;
	virtual bool FS_FindNextFile(HANDLE handle, WIN32_FIND_DATA* dat) = 0;
	virtual bool FS_FindClose(HANDLE handle) = 0;
	virtual int FS_GetSectorSize(FILE*) = 0;
};

extern CFileSystem_Stdio** g_pFullFileSystem; // Ptr to g_pFileSystem_Stdio.
extern CFileSystem_Stdio*  g_pFileSystem_Stdio;

//-----------------------------------------------------------------------------
// Singleton FileSystem
//-----------------------------------------------------------------------------
inline CFileSystem_Stdio* FileSystem()
{
	return (*g_pFullFileSystem);
}

///////////////////////////////////////////////////////////////////////////////
class VFileSystem_Stdio : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogVarAdr("g_pFullFileSystem", reinterpret_cast<uintptr_t>(g_pFullFileSystem));
		LogVarAdr("g_pFileSystem_Stdio", reinterpret_cast<uintptr_t>(g_pFileSystem_Stdio));
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const
	{
		g_pFullFileSystem = g_GameDll.FindPatternSIMD("48 8B 0D ?? ?? ?? ?? 45 33 C0 48 83 C1 08 48 8B 01").ResolveRelativeAddressSelf(0x3, 0x7).RCast<CFileSystem_Stdio**>();

		g_pFileSystem_Stdio = g_GameDll.FindPatternSIMD("48 8D 05 ?? ?? ?? ?? 48 89 05 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ?? 48 89 05 ?? ?? ?? ?? E9 ?? ?? ?? ??")
			.FindPattern("48 89", CMemory::Direction::DOWN, 512, 1).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CFileSystem_Stdio*>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const { }
};
///////////////////////////////////////////////////////////////////////////////

#endif // !FILESYSTEM_H
