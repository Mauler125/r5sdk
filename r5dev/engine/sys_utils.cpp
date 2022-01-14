#include "core/stdafx.h"
#include "core/logdef.h"
#include "tier0/commandline.h"
#include "engine/sys_utils.h"
#ifndef DEDICATED
#include "vgui/CEngineVGui.h"
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
	Sys_Error(buf);
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

	va_list args{};
	va_start(args, fmt);

	vsnprintf(buf, sizeof(buf), fmt, args);

	buf[sizeof(buf) - 1] = 0;
	va_end(args);

	DevMsg(eDLL_T::NONE, "Warning(%d)%s\n", level, buf); // TODO: Color
	return Sys_Warning(level, buf);
}

//-----------------------------------------------------------------------------
// Purpose: Show logs to all console interfaces
// Input  : idx - 
//			*fmt - ... - 
// Output : void DevMsg
//-----------------------------------------------------------------------------
void DevMsg(eDLL_T idx, const char* fmt, ...)
{
	static char buf[1024] = {};

	static std::shared_ptr<spdlog::logger> iconsole = spdlog::get("game_console");
	static std::shared_ptr<spdlog::logger> wconsole = spdlog::get("win_console");
	static std::shared_ptr<spdlog::logger> sqlogger = spdlog::get("dev_message_logger");

	{/////////////////////////////
		va_list args{};
		va_start(args, fmt);

		vsnprintf(buf, sizeof(buf), fmt, args);

		buf[sizeof(buf) - 1] = 0;
		va_end(args);
	}/////////////////////////////

	std::string vmStr = sDLL_T[(int)idx].c_str();
	vmStr.append(buf);

	sqlogger->debug(vmStr);

	if (!g_bSpdLog_UseAnsiClr) { wconsole->debug(vmStr); }
	else
	{
		std::string vmStrAnsi = sANSI_DLL_T[(int)idx].c_str();
		vmStrAnsi.append(buf);
		wconsole->debug(vmStrAnsi);
	}

#ifndef DEDICATED
	g_spd_sys_w_oss.str("");
	g_spd_sys_w_oss.clear();

	iconsole->info(vmStr);

	std::string s = g_spd_sys_w_oss.str();

	g_pLogSystem.AddLog((LogType_t)eDLL_T::ENGINE, s);
	g_pIConsole->m_ivConLog.push_back(Strdup(s.c_str()));
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
}

void SysUtils_Detach()
{
	DetourDetach((LPVOID*)&Sys_Error, &HSys_Error);
	DetourDetach((LPVOID*)&Sys_Warning, &HSys_Warning);
	DetourDetach((LPVOID*)&Sys_LoadAssetHelper, &HSys_LoadAssetHelper);
}
