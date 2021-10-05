#include "pch.h"
#include "hooks.h"
#include "logsystem.h"

namespace Hooks
{
	NET_PrintFuncFn originalNET_PrintFunc = nullptr;
	NET_ReceiveDatagramFn originalNET_ReceiveDatagram = nullptr;
	NET_SendDatagramFn originalNET_SendDatagram = nullptr;
}

static std::ostringstream oss_print;
static auto ostream_sink_print = std::make_shared<spdlog::sinks::ostream_sink_st>(oss_print);
static auto log_sink_print = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/NET_Print.txt", true);

//-----------------------------------------------------------------------------
// Purpose: log the clients signonstate to the console
//-----------------------------------------------------------------------------
void Hooks::NET_PrintFunc(const char* fmt, ...)
{
	static bool initialized = false;
	static char buf[1024];

	oss_print.str("");
	oss_print.clear();

	static spdlog::logger logger("sqvm_print", { log_sink_print, ostream_sink_print });

	if (!initialized)
	{
		log_sink_print->set_level(spdlog::level::debug);
		ostream_sink_print->set_level(spdlog::level::debug);
		logger.set_level(spdlog::level::debug);
		logger.set_pattern("[%S.%e] %v");

		initialized = true;
	}

	va_list args;
	va_start(args, fmt);

	vsnprintf(buf, sizeof(buf), fmt, args);

	buf[sizeof(buf) - 1] = 0;
	va_end(args);

	logger.debug(buf);

	std::string s = oss_print.str();
	const char* c = s.c_str();

	g_LogSystem.AddLog(LogType_t::NATIVE, s);

	Items.push_back(Strdup((const char*)c));
}

//-----------------------------------------------------------------------------
// Purpose: hook and log the receive datagram
//-----------------------------------------------------------------------------
bool Hooks::NET_ReceiveDatagram(int sock, void* inpacket, bool raw)
{
	bool result = originalNET_ReceiveDatagram(sock, inpacket, raw);
	if (result)
	{
		int i = NULL;
		netpacket_t* pkt = (netpacket_t*)inpacket;

		///////////////////////////////////////////////////////////////////////////
		// Log received packet data
		HexDump("[+] NET_ReceiveDatagram", 0, &pkt->data[i], pkt->wiresize);
	}

	return result;
}

//-----------------------------------------------------------------------------
// Purpose: hook and log the send datagram
//-----------------------------------------------------------------------------
unsigned int Hooks::NET_SendDatagram(SOCKET s, const char* buf, int len, int flags)
{
	unsigned int result = originalNET_SendDatagram(s, buf, len, flags);
	if (result)
	{
		///////////////////////////////////////////////////////////////////////////
		// Log transmitted packet data
		HexDump("[+] NET_SendDatagram", 0, buf, len);
	}

	return result;
}