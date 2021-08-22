#include "pch.h"
#include "hooks.h"

namespace Hooks
{
	SVC_Print_ProcessFn originalSVC_Print_Process = nullptr;
}

static std::ostringstream oss_print;
static auto ostream_sink_print = std::make_shared<spdlog::sinks::ostream_sink_st>(oss_print);

//-----------------------------------------------------------------------------
// Purpose: log the console messages sent by server
//-----------------------------------------------------------------------------
bool Hooks::SVC_Print_Process(__int64 thisptr)
{
	// TODO: replace with VT hook
	// This is also a call to the handler and for CLC_ClientInfo::Process
	// CNetMessage::GetTypeID ?? | VTF 12
	if (((__int64 (*)(__int64))(*(void***)thisptr)[12])(thisptr) != 2096)
	{
		return originalSVC_Print_Process(thisptr);
	}

	static bool initialized = false;

	static auto iconsole = spdlog::stdout_logger_mt("svc_print_iconsole"); // in-game console
	static auto wconsole = spdlog::stdout_logger_mt("svc_print_wconsole"); // windows console

	oss_print.str("");
	oss_print.clear();

	if (!initialized)
	{
		iconsole = std::make_shared<spdlog::logger>("ostream", ostream_sink_print);
		iconsole->set_pattern("[%S.%e] %v");
		iconsole->set_level(spdlog::level::info);
		wconsole->set_pattern("[%S.%e] %v");
		wconsole->set_level(spdlog::level::info);
		initialized = true;
	}

	const char* m_szText = *reinterpret_cast<const char**>(thisptr + 0x28);

	iconsole->info(m_szText);
	wconsole->info(m_szText);

	Items.push_back(Strdup(oss_print.str().c_str()));

	return true;
}