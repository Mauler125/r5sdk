#include "core/stdafx.h"
#include "tier1/cvar.h"
#include "filesystem/basefilesystem.h"
#include "filesystem/filesystem.h"

#include "bspfile.h"
#include "engine/modelloader.h"

static ConVar fs_showWarnings("fs_showWarnings", "0", FCVAR_DEVELOPMENTONLY | FCVAR_ACCESSIBLE_FROM_THREADS, "Logs the FileSystem warnings to the console, filtered by 'fs_warning_level' ( !slower! ).", true, 0.f, true, 2.f, "0 = log to file. 1 = 0 + log to console. 2 = 1 + log to notify");

//---------------------------------------------------------------------------------
// Purpose: prints the output of the filesystem based on the warning level
// Input  : *this - 
//			level - 
//			*pFmt - 
//---------------------------------------------------------------------------------
void CBaseFileSystem::Warning(CBaseFileSystem* pFileSystem, FileWarningLevel_t level, const char* pFmt, ...)
{
	if (level >= FileWarningLevel_t::FILESYSTEM_WARNING_REPORTALLACCESSES)
	{
		// Logging reads are very verbose! Explicitly toggle..
		if (!fs_showAllReads->GetBool())
		{
			return;
		}
	}

	va_list args;
	va_start(args, pFmt);
	CoreMsgV(LogType_t::LOG_WARNING, static_cast<LogLevel_t>(fs_showWarnings.GetInt()), eDLL_T::FS, "filesystem", pFmt, args);
	va_end(args);
}

//---------------------------------------------------------------------------------
// Purpose: attempts to load files from disk if exist before loading from VPK/cache
// Input  : *pszFilePath - 
// Output : handle to file on success, NULL on failure
//---------------------------------------------------------------------------------
bool CBaseFileSystem::VCheckDisk(const char* pszFilePath)
{
	// Only load material files from the disk if the mode isn't zero,
	// use -novpk to load valve materials from the disk.
	if (FileSystem()->CheckVPKMode(0) && V_strstr(pszFilePath, ".vmt"))
	{
		return false;
	}

	if (V_IsAbsolutePath(pszFilePath))
	{
		// Skip absolute file paths.
		return false;
	}

	CUtlString filePath;
	filePath.Format("platform/%s", pszFilePath);
	filePath.FixSlashes();
	filePath = filePath.Replace("\\*\\", "");

	if (::FileExists(filePath.Get()))
	{
		return true;
	}

	return false;
}

//---------------------------------------------------------------------------------
// Purpose: loads files from VPK
// Input  : *this - 
//			*pResults - 
//			*pszFilePath - 
// Output : handle to file on success, NULL on failure
//---------------------------------------------------------------------------------
FileHandle_t CBaseFileSystem::VReadFromVPK(CBaseFileSystem* pFileSystem, FileHandle_t pResults, const char* pszFilePath)
{
	if (VCheckDisk(pszFilePath))
	{
		*reinterpret_cast<int64_t*>(pResults) = -1;
		return pResults;
	}

	return CBaseFileSystem__LoadFromVPK(pFileSystem, pResults, pszFilePath);
}

//---------------------------------------------------------------------------------
// Purpose: loads files from cache
// Input  : *this - 
//			*pszFilePath - 
//			*pCache - 
// Output : true if file exists, false otherwise
//---------------------------------------------------------------------------------
bool CBaseFileSystem::VReadFromCache(CBaseFileSystem* pFileSystem, const char* pszFilePath, FileSystemCache* pCache)
{
	if (VCheckDisk(pszFilePath))
	{
		return false;
	}

	bool result = CBaseFileSystem__LoadFromCache(pFileSystem, pszFilePath, pCache);
	return result;
}

//---------------------------------------------------------------------------------
// Purpose: mounts a BSP packfile lump as search path
// Input  : *this - 
//			*pPath - 
//			*pPathID - 
//			*addType - 
//---------------------------------------------------------------------------------
void CBaseFileSystem::VAddMapPackFile(CBaseFileSystem* pFileSystem, const char* pPath, const char* pPathID, SearchPathAdd_t addType)
{
	// Since the mounting of the packfile lump is performed before the BSP header
	// is loaded and parsed, we have to do it here. The internal 'AddMapPackFile'
	// function has been patched to load the fields in the global 's_MapHeader'
	// field, instead of the one that is getting initialized (see r5apex.patch).
	if (s_MapHeader->ident != IDBSPHEADER || s_MapHeader->version != BSPVERSION)
	{
		FileHandle_t hBspFile = FileSystem()->Open(pPath, "rb", pPathID);
		if (hBspFile != FILESYSTEM_INVALID_HANDLE)
		{
			memset(s_MapHeader, '\0', sizeof(BSPHeader_t));
			FileSystem()->Read(s_MapHeader, sizeof(BSPHeader_t), hBspFile);
		}
	}

	// If a lump exists, replace the path pointer with that of the lump so that
	// the internal function loads this instead.
	char lumpPathBuf[MAX_PATH];
	V_snprintf(lumpPathBuf, sizeof(lumpPathBuf), "%s.%.4X.bsp_lump", pPath, LUMP_PAKFILE);

	if (FileSystem()->FileExists(lumpPathBuf, pPathID))
	{
		pPath = lumpPathBuf;
	}

	CBaseFileSystem__AddMapPackFile(pFileSystem, pPath, pPathID, addType);
}

//---------------------------------------------------------------------------------
// Purpose: attempts to mount VPK file for filesystem usage
// Input  : *this - 
//			*pszVpkPath - 
// Output : pointer to VPK on success, NULL on failure
//---------------------------------------------------------------------------------
VPKData_t* CBaseFileSystem::VMountVPKFile(CBaseFileSystem* pFileSystem, const char* pszVpkPath)
{
	int nHandle = CBaseFileSystem__GetMountedVPKHandle(pFileSystem, pszVpkPath);
	VPKData_t* pPakData = CBaseFileSystem__MountVPKFile(pFileSystem, pszVpkPath);

	if (pPakData)
	{
		if (nHandle < 0) // Only log if VPK hasn't been mounted yet.
		{
			::Msg(eDLL_T::FS, "Mounted vpk file: '%s' with handle: '%i'\n", pszVpkPath, pPakData->m_nHandle);
		}
	}
	else // VPK failed to load or does not exist...
	{
		::Warning(eDLL_T::FS, "Unable to mount vpk file: '%s'\n", pszVpkPath);
	}

	return pPakData;
}

//---------------------------------------------------------------------------------
// Purpose: unmount a VPK file
// Input  : *this - 
//			*pszVpkPath - 
// Output : pointer to formatted VPK path string
//---------------------------------------------------------------------------------
const char* CBaseFileSystem::VUnmountVPKFile(CBaseFileSystem* pFileSystem, const char* pszVpkPath)
{
	int nHandle = CBaseFileSystem__GetMountedVPKHandle(pFileSystem, pszVpkPath);
	const char* pRet = CBaseFileSystem__UnmountVPKFile(pFileSystem, pszVpkPath);

	if (nHandle >= 0)
	{
		::Msg(eDLL_T::FS, "Unmounted vpk file: '%s' with handle: '%i'\n", pszVpkPath, nHandle);
	}

	return pRet;
}

//---------------------------------------------------------------------------------
// Purpose: reads a string until its null terminator
// Input  : *pFile - 
// Output : string
//---------------------------------------------------------------------------------
CUtlString CBaseFileSystem::ReadString(FileHandle_t pFile)
{
	CUtlString result;
	char c = '\0';

	do
	{
		Read(&c, sizeof(char), pFile);

		if (c)
			result += c;

	} while (c);

	return result;
}

void VBaseFileSystem::Detour(const bool bAttach) const
{
	DetourSetup(&CBaseFileSystem__Warning, &CBaseFileSystem::Warning, bAttach);
	DetourSetup(&CBaseFileSystem__LoadFromVPK, &CBaseFileSystem::VReadFromVPK, bAttach);
	DetourSetup(&CBaseFileSystem__LoadFromCache, &CBaseFileSystem::VReadFromCache, bAttach);
	DetourSetup(&CBaseFileSystem__AddMapPackFile, &CBaseFileSystem::VAddMapPackFile, bAttach);
	DetourSetup(&CBaseFileSystem__MountVPKFile, &CBaseFileSystem::VMountVPKFile, bAttach);
	DetourSetup(&CBaseFileSystem__UnmountVPKFile, &CBaseFileSystem::VUnmountVPKFile, bAttach);
}

CBaseFileSystem* g_pFileSystem = nullptr;