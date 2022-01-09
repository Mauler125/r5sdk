#include "core/stdafx.h"
#include "core/logdef.h"
#include "tier0/cvar.h"
#include "vpc/basefilesystem.h"
#include "gameui/IConsole.h"

//---------------------------------------------------------------------------------
// Purpose: prints the output of the filesystem based on the warning level
//---------------------------------------------------------------------------------
void HCBaseFileSystem_Warning(void* thisptr, FileWarningLevel_t level, const char* fmt, ...)
{
	if (fs_warning_level_native->GetInt() < (int)level)
	{
		return;
	}

	static bool initialized = false;
	static char buf[1024] = {};

	static auto iconsole = spdlog::stdout_logger_mt("fs_warn_iconsole"); // in-game console.
	static auto wconsole = spdlog::stdout_logger_mt("fs_warn_wconsole"); // windows console.

	fs_oss.str("");
	fs_oss.clear();

	if (!initialized)
	{
		iconsole = std::make_shared<spdlog::logger>("fs_warn_ostream", fs_ostream_sink);
		iconsole->set_pattern("[%S.%e] %v");
		iconsole->set_level(spdlog::level::debug);
		wconsole->set_pattern("[%S.%e] %v");
		wconsole->set_level(spdlog::level::debug);
		initialized = true;
	}

	va_list args{};
	va_start(args, fmt);

	vsnprintf(buf, sizeof(buf), fmt, args);

	buf[sizeof(buf) - 1] = 0;
	va_end(args);

	iconsole->debug(buf);
	wconsole->debug(buf);

#ifndef DEDICATED
	std::string s = fs_oss.str();
	const char* c = s.c_str();

	Items.push_back(Strdup((const char*)c));
#endif // !DEDICATED
}

void CBaseFileSystem_Attach()
{
	DetourAttach((LPVOID*)&CBaseFileSystem_Warning, &HCBaseFileSystem_Warning);
}

void CBaseFileSystem_Detach()
{
	DetourDetach((LPVOID*)&CBaseFileSystem_Warning, &HCBaseFileSystem_Warning);
}
