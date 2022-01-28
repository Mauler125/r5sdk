#include "core/stdafx.h"
#include "core/logdef.h"
#include "tier0/cvar.h"
#include "vpc/basefilesystem.h"
#ifndef DEDICATED
#include "gameui/IConsole.h"
#endif // !DEDICATED

//---------------------------------------------------------------------------------
// Purpose: prints the output of the filesystem based on the warning level
//---------------------------------------------------------------------------------
void HCBaseFileSystem_Warning(void* thisptr, FileWarningLevel_t level, const char* fmt, ...)
{
	if (fs_warning_level_sdk->GetInt() < (int)level)
	{
		return;
	}

	static char buf[1024] = {};

	static std::shared_ptr<spdlog::logger> iconsole = spdlog::get("game_console");
	static std::shared_ptr<spdlog::logger> wconsole = spdlog::get("win_console");
	static std::shared_ptr<spdlog::logger> fslogger = spdlog::get("filesystem_warn_logger");

	{/////////////////////////////
		va_list args{};
		va_start(args, fmt);

		vsnprintf(buf, sizeof(buf), fmt, args);

		buf[sizeof(buf) - 1] = 0;
		va_end(args);
	}/////////////////////////////

	fslogger->debug(buf);

	if (fs_show_warning_output->GetBool())
	{
		wconsole->debug(buf);
#ifndef DEDICATED
		g_spd_sys_w_oss.str("");
		g_spd_sys_w_oss.clear();

		iconsole->debug(buf);

		std::string s = g_spd_sys_w_oss.str();

		g_pIConsole->m_ivConLog.push_back(Strdup(s.c_str()));
#endif // !DEDICATED
	}
}

//---------------------------------------------------------------------------------
// Purpose: attempts to load files from disk if exist before loading from VPK
//---------------------------------------------------------------------------------
FileHandle_t HCBaseFileSystem_ReadFromVPK(void* pVpk, std::int64_t* pResults, char* pszFilePath)
{
	std::string svFilePath = ConvertToWinPath(pszFilePath);

	if (strstr(svFilePath.c_str(), "\\\*\\"))
	{
		// Erase '//*/'.
		svFilePath.erase(0, 4);
	}

	// TODO: obtain 'mod' SearchPath's instead.
	svFilePath.insert(0, "platform\\");

	if (FileExists(svFilePath.c_str()) /*|| FileExists(pszFilePath)*/)
	{
		*pResults = -1;
		return (void*)pResults;
	}
	return CBaseFileSystem_LoadFromVPK(pVpk, pResults, pszFilePath);
}

//---------------------------------------------------------------------------------
// Purpose: attempts to load files from disk if exist before loading from cache
//---------------------------------------------------------------------------------
bool HCBaseFileSystem_ReadFromCache(void* pFileSystem, char* pszFilePath, void* pResults)
{
	std::string svFilePath = ConvertToWinPath(pszFilePath);

	if (strstr(svFilePath.c_str(), "\\\*\\"))
	{
		// Erase '//*/'.
		svFilePath.erase(0, 4);
	}

	// TODO: obtain 'mod' SearchPath's instead.
	svFilePath.insert(0, "platform\\");

	if (FileExists(svFilePath.c_str()) /*|| FileExists(pszFilePath)*/)
	{
		return false;
	}
	return CBaseFileSystem_LoadFromCache(pFileSystem, pszFilePath, pResults);
}

void CBaseFileSystem_Attach()
{
	DetourAttach((LPVOID*)&CBaseFileSystem_Warning, &HCBaseFileSystem_Warning);
	DetourAttach((LPVOID*)&CBaseFileSystem_LoadFromVPK, &HCBaseFileSystem_ReadFromVPK);
	DetourAttach((LPVOID*)&CBaseFileSystem_LoadFromCache, &HCBaseFileSystem_ReadFromCache);
}

void CBaseFileSystem_Detach()
{
	DetourDetach((LPVOID*)&CBaseFileSystem_Warning, &HCBaseFileSystem_Warning);
	DetourDetach((LPVOID*)&CBaseFileSystem_LoadFromVPK, &HCBaseFileSystem_ReadFromVPK);
	DetourDetach((LPVOID*)&CBaseFileSystem_LoadFromCache, &HCBaseFileSystem_ReadFromCache);
}
