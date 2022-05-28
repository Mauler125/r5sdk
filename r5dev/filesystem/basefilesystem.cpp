#include "core/stdafx.h"
#include "core/logdef.h"
#include "tier1/cvar.h"
#include "filesystem/basefilesystem.h"
#include "filesystem/filesystem.h"
#ifndef DEDICATED
#include "gameui/IConsole.h"
#endif // !DEDICATED

//---------------------------------------------------------------------------------
// Purpose: reads data from memory
// Input  : *pOutput - 
//			nSize - 
//			hFile - 
// Output : lenght of read data
//---------------------------------------------------------------------------------
int CBaseFileSystem::Read(void* pOutput, int nSize, FileHandle_t hFile)
{
	int index = 0;
	return CallVFunc<int>(index, this, pOutput, nSize, hFile);
}

//---------------------------------------------------------------------------------
// Purpose: open file
// Input  : *pFileName - 
//			*pOptions - 
//			*pPathID - 
//			unknown
// Output : Handle to file on success, NULL on failure
//---------------------------------------------------------------------------------
FileHandle_t CBaseFileSystem::Open(const char* pFileName, const char* pOptions, const char* pPathID, int64_t unknown)
{
	int index = 2;
	return CallVFunc<FileHandle_t>(index, this, pFileName, pOptions, pPathID, unknown);
}

//---------------------------------------------------------------------------------
// Purpose: close file by handle
// Input  : file - 
//---------------------------------------------------------------------------------
void CBaseFileSystem::Close(FileHandle_t file)
{
	int index = 3;
	CallVFunc<void>(index, this, file);
}

//---------------------------------------------------------------------------------
// Purpose: checks if file exists in all searchpaths and pak files
// Input  : *pFileName - 
//			*pPathID - 
// Output : true if file exists, false otherwise
//---------------------------------------------------------------------------------
bool CBaseFileSystem::FileExists(const char* pFileName, const char* pPathID)
{
	int index = 10;
	return CallVFunc<bool>(index, this, pFileName, pPathID);
}

//---------------------------------------------------------------------------------
// Purpose: prints the output of the filesystem based on the warning level
// Input  : *this - 
//			level - 
//			*pFmt - 
//---------------------------------------------------------------------------------
void CBaseFileSystem::Warning(CBaseFileSystem* pFileSystem, FileWarningLevel_t level, const char* pFmt, ...)
{
	if (fs_warning_level_sdk->GetInt() < static_cast<int>(level))
	{
		return;
	}

	static char szBuf[1024] = {};

	static std::shared_ptr<spdlog::logger> iconsole = spdlog::get("game_console");
	static std::shared_ptr<spdlog::logger> wconsole = spdlog::get("win_console");
	static std::shared_ptr<spdlog::logger> fslogger = spdlog::get("fs_warn");

	{/////////////////////////////
		va_list args{};
		va_start(args, pFmt);

		vsnprintf(szBuf, sizeof(szBuf), pFmt, args);

		szBuf[sizeof(szBuf) - 1] = 0;
		va_end(args);
	}/////////////////////////////

	fslogger->debug(szBuf);

	if (fs_show_warning_output->GetBool())
	{
		wconsole->debug(szBuf);
#ifndef DEDICATED
		iconsole->debug(szBuf);
		g_pConsole->m_ivConLog.push_back(CConLog(g_spd_sys_w_oss.str(), ImVec4(1.00f, 1.00f, 0.00f, 1.00f)));

		g_spd_sys_w_oss.str("");
		g_spd_sys_w_oss.clear();
#endif // !DEDICATED
	}
}

//---------------------------------------------------------------------------------
// Purpose: attempts to load files from disk if exist before loading from VPK
// Input  : *pVpk - 
//			*pResults - 
//			*pszFilePath - 
// Output : Handle to file on success, NULL on failure
//---------------------------------------------------------------------------------
FileHandle_t CBaseFileSystem::ReadFromVPK(CBaseFileSystem* pFileSystem, std::int64_t* pResults, char* pszFilePath)
{
	std::string svFilePath = ConvertToWinPath(pszFilePath);

	if (strstr(svFilePath.c_str(), "\\\*\\"))
	{
		// Erase '//*/'.
		svFilePath.erase(0, 4);
	}

	// TODO: obtain 'mod' SearchPath's instead.
	svFilePath.insert(0, "platform\\");

	if (::FileExists(svFilePath.c_str()) /*|| ::FileExists(pszFilePath)*/)
	{
		*pResults = -1;
		return (void*)pResults;
	}
	return CBaseFileSystem_LoadFromVPK(pFileSystem, pResults, pszFilePath);
}

//---------------------------------------------------------------------------------
// Purpose: attempts to load files from disk if exist before loading from cache
// Input  : *pFileSystem - 
//			*pszFilePath - 
//			*pResults - 
// Output : true if file exists, false otherwise
//---------------------------------------------------------------------------------
bool CBaseFileSystem::ReadFromCache(CBaseFileSystem* pFileSystem, char* pszFilePath, void* pResults)
{
	std::string svFilePath = ConvertToWinPath(pszFilePath);

	if (strstr(svFilePath.c_str(), "\\\*\\"))
	{
		// Erase '//*/'.
		svFilePath.erase(0, 4);
	}

	// TODO: obtain 'mod' SearchPath's instead.
	svFilePath.insert(0, "platform\\");

	if (::FileExists(svFilePath.c_str()) /*|| ::FileExists(pszFilePath)*/)
	{
		return false;
	}
	return CBaseFileSystem_LoadFromCache(pFileSystem, pszFilePath, pResults);
}

//-----------------------------------------------------------------------------
// Purpose: create the search path.
// Input  : *pPath - 
//			*pPathID - 
//			addType - 
//-----------------------------------------------------------------------------
void CBaseFileSystem::AddSearchPath(CBaseFileSystem* pFileSystem, const char* pPath, const char* pPathID, SearchPathAdd_t addType)
{
	CBaseFileSystem_AddSearchPath(pFileSystem, pPath, pPathID, addType);
}

//-----------------------------------------------------------------------------
// Purpose: remove the search path.
// Input  : *pPath - 
//			*pPathID - 
//			addType - 
// Output : true on success, false otherwise.
//-----------------------------------------------------------------------------
bool CBaseFileSystem::RemoveSearchPath(CBaseFileSystem* pFileSystem, const char* pPath, const char* pPathID)
{
	return CBaseFileSystem_RemoveSearchPath(pFileSystem, pPath, pPathID);
}

void CBaseFileSystem_Attach()
{
	DetourAttach((LPVOID*)&CBaseFileSystem_Warning, &CBaseFileSystem::Warning);
	DetourAttach((LPVOID*)&CBaseFileSystem_LoadFromVPK, &CBaseFileSystem::ReadFromVPK);
	DetourAttach((LPVOID*)&CBaseFileSystem_LoadFromCache, &CBaseFileSystem::ReadFromCache);
	DetourAttach((LPVOID*)&CBaseFileSystem_AddSearchPath, &CBaseFileSystem::AddSearchPath);
	DetourAttach((LPVOID*)&CBaseFileSystem_RemoveSearchPath, &CBaseFileSystem::RemoveSearchPath);
}

void CBaseFileSystem_Detach()
{
	DetourDetach((LPVOID*)&CBaseFileSystem_Warning, &CBaseFileSystem::Warning);
	DetourDetach((LPVOID*)&CBaseFileSystem_LoadFromVPK, &CBaseFileSystem::ReadFromVPK);
	DetourDetach((LPVOID*)&CBaseFileSystem_LoadFromCache, &CBaseFileSystem::ReadFromCache);
	DetourDetach((LPVOID*)&CBaseFileSystem_AddSearchPath, &CBaseFileSystem::AddSearchPath);
	DetourDetach((LPVOID*)&CBaseFileSystem_RemoveSearchPath, &CBaseFileSystem::RemoveSearchPath);
}
CBaseFileSystem* g_pFileSystem = nullptr;