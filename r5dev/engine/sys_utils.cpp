#include "core/stdafx.h"
#include "core/logdef.h"
#include "engine/sys_utils.h"
#include "vgui/CEngineVGui.h"
#include "gameui/IConsole.h"

//-----------------------------------------------------------------------------
//	Sys_Error
//
//-----------------------------------------------------------------------------
void HSys_Error(char* fmt, ...)
{
	static char buf[1024];

	va_list args;
	va_start(args, fmt);

	vsnprintf(buf, sizeof(buf), fmt, args);

	buf[sizeof(buf) -1] = 0;
	va_end(args);

	DevMsg(eDLL_T::ENGINE, "%s\n", buf);
	Sys_Error(buf);
}

//-----------------------------------------------------------------------------
//	Sys_Print
//
//-----------------------------------------------------------------------------
void DevMsg(eDLL_T idx, const char* fmt, ...)
{
	int vmIdx = (int)idx;
	static bool initialized = false;
	static char buf[1024];

	static std::string vmType[7] = { "Native(S):", "Native(C):", "Native(U):", "Native(E):", "Native(F):", "Native(R):", "Native(M):" };

	static auto iconsole = spdlog::stdout_logger_mt("sys_print_iconsole"); // in-game console.
	static auto wconsole = spdlog::stdout_logger_mt("sys_print_wconsole"); // windows console.
	static auto sqlogger = spdlog::basic_logger_mt("sys_print_logger", "platform\\logs\\sys_print.log"); // file logger.

	std::string vmStr = vmType[vmIdx].c_str();

	g_spd_sys_w_oss.str("");
	g_spd_sys_w_oss.clear();

	if (!initialized)
	{
		iconsole = std::make_shared<spdlog::logger>("sys_print_ostream", g_spd_sys_p_ostream_sink);
		iconsole->set_pattern("[%S.%e] %v");
		iconsole->set_level(spdlog::level::debug);
		wconsole->set_pattern("[%S.%e] %v");
		wconsole->set_level(spdlog::level::debug);
		sqlogger->set_pattern("[%S.%e] %v");
		sqlogger->set_level(spdlog::level::debug);
		initialized = true;
	}

	va_list args;
	va_start(args, fmt);

	vsnprintf(buf, sizeof(buf), fmt, args);

	buf[sizeof(buf) - 1] = 0;
	va_end(args);

	vmStr.append(buf);

	iconsole->debug(vmStr);
	wconsole->debug(vmStr);
	sqlogger->debug(vmStr);

#ifndef DEDICATED
	std::string s = g_spd_sys_w_oss.str();
	const char* c = s.c_str();

	g_pLogSystem.AddLog((LogType_t)eDLL_T::ENGINE, s);
	Items.push_back(Strdup((const char*)c));
#endif // !DEDICATED
}

//-----------------------------------------------------------------------------
//	Sys_LoadAssetHelper
//
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
	DetourAttach((LPVOID*)&Sys_LoadAssetHelper, &HSys_LoadAssetHelper);
}

void SysUtils_Detach()
{
	DetourDetach((LPVOID*)&Sys_Error, &HSys_Error);
	DetourDetach((LPVOID*)&Sys_LoadAssetHelper, &HSys_LoadAssetHelper);
}
