#include "core/stdafx.h"
#include "core/logdef.h"
#include "tier1/cvar.h"
#include "filesystem/basefilesystem.h"
#include "filesystem/filesystem.h"
#ifndef DEDICATED
#include "gameui/IConsole.h"
#endif // !DEDICATED

//---------------------------------------------------------------------------------
// Purpose: prints the output of the filesystem based on the warning level
// Input  : *this - 
//			level - 
//			*pFmt - 
//---------------------------------------------------------------------------------
void CBaseFileSystem::Warning(CBaseFileSystem* pFileSystem, FileWarningLevel_t level, const char* pFmt, ...)
{
	va_list args;
	va_start(args, pFmt);
	CoreMsgV(LogType_t::LOG_WARNING, static_cast<LogLevel_t>(fs_show_warnings->GetInt()), eDLL_T::FS, "filesystem", pFmt, args);
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
	if (FileSystem()->CheckVPKMode(0) && strstr(pszFilePath, ".vmt"))
	{
		return false;
	}

	std::string svFilePath = ConvertToWinPath(pszFilePath);
	if (svFilePath.find("\\*\\") != string::npos)
	{
		// Erase '//*/'.
		svFilePath.erase(0, 4);
	}

	fs::path filePath(svFilePath);
	if (filePath.is_absolute())
	{
		// Skip absolute file paths.
		return false;
	}

	// TODO: obtain 'mod' SearchPath's instead.
	svFilePath.insert(0, "platform\\");

	if (::FileExists(svFilePath) /*|| ::FileExists(pszFilePath)*/)
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
FileHandle_t CBaseFileSystem::VReadFromVPK(CBaseFileSystem* pFileSystem, FileHandle_t pResults, char* pszFilePath)
{
	if (VCheckDisk(pszFilePath))
	{
		*reinterpret_cast<int64_t*>(pResults) = -1;
		return pResults;
	}

	return v_CBaseFileSystem_LoadFromVPK(pFileSystem, pResults, pszFilePath);
}

//---------------------------------------------------------------------------------
// Purpose: loads files from cache
// Input  : *this - 
//			*pszFilePath - 
//			*pResults - 
// Output : true if file exists, false otherwise
//---------------------------------------------------------------------------------
bool CBaseFileSystem::VReadFromCache(CBaseFileSystem* pFileSystem, char* pszFilePath, void* pResults)
{
	if (VCheckDisk(pszFilePath))
	{
		return false;
	}

	return v_CBaseFileSystem_LoadFromCache(pFileSystem, pszFilePath, pResults);
}

//---------------------------------------------------------------------------------
// Purpose: attempts to mount VPK file for filesystem usage
// Input  : *this - 
//			*pszVpkPath - 
// Output : pointer to VPK on success, NULL on failure
//---------------------------------------------------------------------------------
VPKData_t* CBaseFileSystem::VMountVPKFile(CBaseFileSystem* pFileSystem, const char* pszVpkPath)
{
	int nHandle = v_CBaseFileSystem_GetMountedVPKHandle(pFileSystem, pszVpkPath);
	VPKData_t* pPakData = v_CBaseFileSystem_MountVPKFile(pFileSystem, pszVpkPath);

	if (pPakData)
	{
		if (nHandle < 0) // Only log if VPK hasn't been mounted yet.
		{
			::DevMsg(eDLL_T::FS, "Mounted vpk file: '%s' with handle: '%i'\n", pszVpkPath, pPakData->m_nHandle);
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
	int nHandle = v_CBaseFileSystem_GetMountedVPKHandle(pFileSystem, pszVpkPath);
	const char* pRet = v_CBaseFileSystem_UnmountVPKFile(pFileSystem, pszVpkPath);

	if (nHandle >= 0)
	{
		::DevMsg(eDLL_T::FS, "Unmounted vpk file: '%s' with handle: '%i'\n", pszVpkPath, nHandle);
	}

	return pRet;
}

//---------------------------------------------------------------------------------
// Purpose: reads a string until its null terminator
// Input  : *pFile - 
// Output : string
//---------------------------------------------------------------------------------
string CBaseFileSystem::ReadString(FileHandle_t pFile)
{
	string svString;
	char c = '\0';

	do
	{
		Read(&c, sizeof(char), pFile);

		if (c)
			svString += c;

	} while (c);

	return svString;
}

void VBaseFileSystem::Attach() const
{
	DetourAttach((LPVOID*)&v_CBaseFileSystem_Warning, &CBaseFileSystem::Warning);
	DetourAttach((LPVOID*)&v_CBaseFileSystem_LoadFromVPK, &CBaseFileSystem::VReadFromVPK);
	DetourAttach((LPVOID*)&v_CBaseFileSystem_LoadFromCache, &CBaseFileSystem::VReadFromCache);
	DetourAttach((LPVOID*)&v_CBaseFileSystem_MountVPKFile, &CBaseFileSystem::VMountVPKFile);
	DetourAttach((LPVOID*)&v_CBaseFileSystem_UnmountVPKFile, &CBaseFileSystem::VUnmountVPKFile);
}

void VBaseFileSystem::Detach() const
{
	DetourDetach((LPVOID*)&v_CBaseFileSystem_Warning, &CBaseFileSystem::Warning);
	DetourDetach((LPVOID*)&v_CBaseFileSystem_LoadFromVPK, &CBaseFileSystem::VReadFromVPK);
	DetourDetach((LPVOID*)&v_CBaseFileSystem_LoadFromCache, &CBaseFileSystem::VReadFromCache);
	DetourDetach((LPVOID*)&v_CBaseFileSystem_MountVPKFile, &CBaseFileSystem::VMountVPKFile);
	DetourDetach((LPVOID*)&v_CBaseFileSystem_UnmountVPKFile, &CBaseFileSystem::VUnmountVPKFile);
}
CBaseFileSystem* g_pFileSystem = nullptr;