//=============================================================================//
//
// Purpose: General system utilities.
//
//=============================================================================//

#include "core/stdafx.h"
#include "core/logdef.h"
#include "tier0/cvar.h"
#include "tier0/commandline.h"
#include "engine/common.h"
#include "engine/host_state.h"
#include "engine/sys_utils.h"
#include "engine/cmodel_bsp.h"
#ifdef DEDICATED
#include "engine/sv_rcon.h"
#else
#include "vgui/vgui_debugpanel.h"
#include "gameui/IConsole.h"
#endif // !DEDICATED

//-----------------------------------------------------------------------------
// Purpose: Exit engine with error
// Input  : *error - 
//			... - 
// Output : void Sys_Error
//-----------------------------------------------------------------------------
void HSys_Error(char* fmt, ...)
{
	static char buf[1024] = {};

	va_list args{};
	va_start(args, fmt);

	vsnprintf(buf, sizeof(buf), fmt, args);

	buf[sizeof(buf) -1] = 0;
	va_end(args);

	DevMsg(eDLL_T::ENGINE, "%s\n", buf);
	return Sys_Error(buf);
}

//-----------------------------------------------------------------------------
// Purpose: Show warning in the console, exit engine with error when level 5
// Input  : level -
//			*error - ... - 
// Output : void* Sys_Warning
//-----------------------------------------------------------------------------
void* HSys_Warning(int level, char* fmt, ...)
{
	static char buf[1024] = {};
	{/////////////////////////////
		va_list args{};
		va_start(args, fmt);

		vsnprintf(buf, sizeof(buf), fmt, args);

		buf[sizeof(buf) - 1] = 0;
		va_end(args);
	}/////////////////////////////

	DevMsg(eDLL_T::NONE, "Warning(%d):%s\n", level, buf); // TODO: Color
	return Sys_Warning(level, buf);
}

#ifndef DEDICATED
//-----------------------------------------------------------------------------
// Purpose: Builds log to be displayed on the screen
// Input  : pos - 
//			*fmt - ... - 
// Output : void NPrintf
//-----------------------------------------------------------------------------
void HCon_NPrintf(int pos, const char* fmt, ...)
{
	if (cl_showhoststats->GetBool())
	{
		static char buf[1024] = {};
		{/////////////////////////////
			va_list args{};
			va_start(args, fmt);

			vsnprintf(buf, sizeof(buf), fmt, args);

			buf[sizeof(buf) - 1] = 0;
			va_end(args);
		}/////////////////////////////

		snprintf((char*)g_pLogSystem.m_pszCon_NPrintf_Buf, 4096, buf);
	}
}
#endif // !DEDICATED

//-----------------------------------------------------------------------------
// Purpose: Show logs to all console interfaces
// Input  : idx - 
//			*fmt - ... - 
// Output : void DevMsg
//-----------------------------------------------------------------------------
void DevMsg(eDLL_T idx, const char* fmt, ...)
{
	static char szBuf[1024] = {};

	static std::string svOut;
	static std::string svAnsiOut;

	static std::regex rxAnsiExp("\\\033\\[.*?m");

	static std::shared_ptr<spdlog::logger> iconsole = spdlog::get("game_console");
	static std::shared_ptr<spdlog::logger> wconsole = spdlog::get("win_console");
	static std::shared_ptr<spdlog::logger> sqlogger = spdlog::get("dev_message_logger");

	{/////////////////////////////
		va_list args{};
		va_start(args, fmt);

		vsnprintf(szBuf, sizeof(szBuf), fmt, args);

		szBuf[sizeof(szBuf) - 1] = 0;
		va_end(args);
	}/////////////////////////////

	svOut = sDLL_T[static_cast<int>(idx)].c_str();
	svOut.append(szBuf);
	svOut = std::regex_replace(svOut, rxAnsiExp, "");

	char szNewLine = svOut.back();
	if (szNewLine != '\n')
	{
		svOut.append("\n");
	}

	if (!g_bSpdLog_UseAnsiClr)
	{
		wconsole->debug(svOut);
#ifdef DEDICATED
		g_pRConServer->Send(svOut.c_str());
#endif // DEDICATED
	}
	else
	{
		svAnsiOut = sANSI_DLL_T[static_cast<int>(idx)].c_str();
		svAnsiOut.append(szBuf);

		char szNewLine = svAnsiOut.back();
		if (szNewLine != '\n')
		{
			svAnsiOut.append("\n");
		}
		wconsole->debug(svAnsiOut);
#ifdef DEDICATED
		g_pRConServer->Send(svAnsiOut.c_str());
#endif // DEDICATED
	}

	sqlogger->debug(svOut);

#ifndef DEDICATED
	iconsole->info(svOut);
	std::string s = g_spd_sys_w_oss.str();

	int nLog = static_cast<int>(idx) + 3; // RUI log enum is shifted by 3 for scripts.
	LogType_t tLog = static_cast<LogType_t>(nLog);

	g_pLogSystem.AddLog(tLog, s);
	g_pIConsole->m_ivConLog.push_back(Strdup(s.c_str()));

	g_spd_sys_w_oss.str("");
	g_spd_sys_w_oss.clear();
#endif // !DEDICATED
}

//-----------------------------------------------------------------------------
// Purpose: Load assets from a custom directory if file exists
// Input  : *lpFileName - 
//			a2 - *a3 - 
// Output : void* Sys_LoadAssetHelper
//-----------------------------------------------------------------------------
void* HSys_LoadAssetHelper(const CHAR* lpFileName, std::int64_t a2, LARGE_INTEGER* a3)
{
	std::string mod_file;
	std::string base_file = lpFileName;
	const std::string mod_dir = "paks\\Win32\\";
	const std::string base_dir = "paks\\Win64\\";
	static bool bBasePaksLoaded = false;

	if (g_pHostState)
	{
		std::string svLevelName = g_pHostState->m_levelName;
		std::string svMapPakName = svLevelName + ".rpak";

		if (!g_bLevelResourceInitialized && !g_pHostState->m_bActiveGame &&
			bBasePaksLoaded || !strcmp(std::string(lpFileName).erase(0, 11).c_str(), "mp_lobby.rpak"))
		{
			// Attempt to load level dependencies if they exist.
			MOD_LoadDependencies(eBspRes_t::RES_RPAK);

			// By the time mp_lobby.rpak is loaded, all the base paks are loaded as well and we can load anything else.
			bBasePaksLoaded = true;
			g_bLevelResourceInitialized = true;
		}
	}

	if (strstr(lpFileName, base_dir.c_str()))
	{
		base_file.erase(0, 11); // Erase 'base_dir'.
		mod_file = mod_dir + base_file; // Prepend 'mod_dir'.

		if (FileExists(mod_file.c_str()))
		{
			// Load decompressed pak files from 'mod_dir'.
			DevMsg(eDLL_T::RTECH, "Loading pak: '%s'\n", mod_file.c_str());
			return Sys_LoadAssetHelper(mod_file.c_str(), a2, a3);
		}
	}
	if (strstr(lpFileName, base_dir.c_str()))
	{
		DevMsg(eDLL_T::RTECH, "Loading pak: '%s'\n", lpFileName);
	}
	return Sys_LoadAssetHelper(lpFileName, a2, a3);
}

void SysUtils_Attach()
{
	DetourAttach((LPVOID*)&Sys_Error, &HSys_Error);
	DetourAttach((LPVOID*)&Sys_Warning, &HSys_Warning);
	DetourAttach((LPVOID*)&Sys_LoadAssetHelper, &HSys_LoadAssetHelper);
#ifndef DEDICATED
	DetourAttach((LPVOID*)&Con_NPrintf, &HCon_NPrintf);
#endif // !DEDICATED
}

void SysUtils_Detach()
{
	DetourDetach((LPVOID*)&Sys_Error, &HSys_Error);
	DetourDetach((LPVOID*)&Sys_Warning, &HSys_Warning);
	DetourDetach((LPVOID*)&Sys_LoadAssetHelper, &HSys_LoadAssetHelper);
#ifndef DEDICATED
	DetourDetach((LPVOID*)&Con_NPrintf, &HCon_NPrintf);
#endif // !DEDICATED
}
