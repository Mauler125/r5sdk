//==== Copyright (c) 1996-2005, Valve Corporation, All rights reserved. =====//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#include "core/stdafx.h"
#include "core/logdef.h"
#include "tier0/dbg.h"
#include "tier0/platform.h"
#include "tier0/threadtools.h"
#include <tier0/commandline.h>
#ifndef DEDICATED
#include "vgui/vgui_debugpanel.h"
#include "gameui/IConsole.h"
#else
#include "engine/server/sv_rcon.h"
#endif

#if defined( _X360 )
#include "xbox/xbox_console.h"
#endif
std::mutex s_LogMutex;

//-----------------------------------------------------------------------------
// True if -hushasserts was passed on command line.
//-----------------------------------------------------------------------------
bool HushAsserts()
{
#ifdef DBGFLAG_ASSERT
	static bool s_bHushAsserts = !!CommandLine()->FindParm("-hushasserts");
	return s_bHushAsserts;
#else
	return true;
#endif
}

//-----------------------------------------------------------------------------
// Templates to assist in validating pointers:
//-----------------------------------------------------------------------------
PLATFORM_INTERFACE void _AssertValidReadPtr(void* ptr, int count/* = 1*/)
{
#if defined( _WIN32 ) && !defined( _X360 )
	Assert(!IsBadReadPtr(ptr, count));
#else
	Assert(!count || ptr);
#endif
}

PLATFORM_INTERFACE void _AssertValidWritePtr(void* ptr, int count/* = 1*/)
{
#if defined( _WIN32 ) && !defined( _X360 )
	Assert(!IsBadWritePtr(ptr, count));
#else
	Assert(!count || ptr);
#endif
}

PLATFORM_INTERFACE void _AssertValidReadWritePtr(void* ptr, int count/* = 1*/)
{
#if defined( _WIN32 ) && !defined( _X360 )
	Assert(!(IsBadWritePtr(ptr, count) || IsBadReadPtr(ptr, count)));
#else
	Assert(!count || ptr);
#endif
}

PLATFORM_INTERFACE void _AssertValidStringPtr(const TCHAR* ptr, int maxchar/* = 0xFFFFFF */)
{
#if defined( _WIN32 ) && !defined( _X360 )
#ifdef TCHAR_IS_CHAR
	Assert(!IsBadStringPtr(ptr, maxchar));
#else
	Assert(!IsBadStringPtrW(ptr, maxchar));
#endif
#else
	Assert(ptr);
#endif
}

PLATFORM_INTERFACE void AssertValidWStringPtr(const wchar_t* ptr, int maxchar/* = 0xFFFFFF */)
{
#if defined( _WIN32 ) && !defined( _X360 )
	Assert(!IsBadStringPtrW(ptr, maxchar));
#else
	Assert(ptr);
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Show logs to all console interfaces
// Input  : idx - 
//			*fmt - ... - 
//-----------------------------------------------------------------------------
void DevMsg(eDLL_T context, const char* fmt, ...)
{
	static char szBuf[4096] = {};

	static std::string svOut;
	static std::string svAnsiOut;

	static std::regex rxAnsiExp("\\\033\\[.*?m");

	static std::shared_ptr<spdlog::logger> iconsole = spdlog::get("game_console");
	static std::shared_ptr<spdlog::logger> wconsole = spdlog::get("win_console");
	static std::shared_ptr<spdlog::logger> sqlogger = spdlog::get("sdk_info");

	s_LogMutex.lock();
	{/////////////////////////////
		va_list args{};
		va_start(args, fmt);

		vsnprintf(szBuf, sizeof(szBuf), fmt, args);

		szBuf[sizeof(szBuf) - 1] = 0;
		va_end(args);
	}/////////////////////////////

	svOut = Plat_GetProcessUpTime();
	svOut.append(sDLL_T[static_cast<int>(context)]);
	svOut.append(szBuf);
	svOut = std::regex_replace(svOut, rxAnsiExp, "");

	if (svOut.back() != '\n')
	{
		svOut.append("\n");
	}

	if (!g_bSpdLog_UseAnsiClr)
	{
		wconsole->debug(svOut);
#ifdef DEDICATED
		RCONServer()->Send(RCONServer()->Serialize(svOut, "", sv_rcon::response_t::SERVERDATA_RESPONSE_CONSOLE_LOG));
#endif // DEDICATED
	}
	else
	{
		svAnsiOut = Plat_GetProcessUpTime();
		svAnsiOut.append(sANSI_DLL_T[static_cast<int>(context)]);
		svAnsiOut.append(szBuf);

		if (svAnsiOut.back() != '\n')
		{
			svAnsiOut.append("\n");
		}
		wconsole->debug(svAnsiOut);
#ifdef DEDICATED
		RCONServer()->Send(RCONServer()->Serialize(svAnsiOut, "", sv_rcon::response_t::SERVERDATA_RESPONSE_CONSOLE_LOG));
#endif // DEDICATED
	}

	sqlogger->debug(svOut);

#ifndef DEDICATED
	iconsole->info(svOut);

	int nLog = static_cast<int>(context) + 3; // RUI log enum is shifted by 3 for scripts.
	LogType_t tLog = static_cast<LogType_t>(nLog);

	ImVec4 color;
	switch (context)
	{
	case eDLL_T::SERVER:
		color = ImVec4(0.23f, 0.47f, 0.85f, 1.00f);
		break;
	case eDLL_T::CLIENT:
		color = ImVec4(0.46f, 0.46f, 0.46f, 1.00f);
		break;
	case eDLL_T::UI:
		color = ImVec4(0.59f, 0.35f, 0.46f, 1.00f);
		break;
	case eDLL_T::ENGINE:
		color = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
		break;
	case eDLL_T::FS:
		color = ImVec4(0.32f, 0.64f, 0.72f, 1.00f);
		break;
	case eDLL_T::RTECH:
		color = ImVec4(0.36f, 0.70f, 0.35f, 1.00f);
		break;
	case eDLL_T::MS:
		color = ImVec4(0.75f, 0.41f, 0.67f, 1.00f);
		break;
	default:
		color = ImVec4(0.81f, 0.81f, 0.81f, 1.00f);
		break;
	}

	g_pConsole->AddLog(ConLog_t(g_spd_sys_w_oss.str(), color));
	g_pLogSystem.AddLog(tLog, g_spd_sys_w_oss.str());

	g_spd_sys_w_oss.str("");
	g_spd_sys_w_oss.clear();
#endif // !DEDICATED
	s_LogMutex.unlock();
}

//-----------------------------------------------------------------------------
// Purpose: Print engine and SDK errors
// Input  : idx - 
//			*fmt - ... - 
//-----------------------------------------------------------------------------
void Warning(eDLL_T context, const char* fmt, ...)
{
	static char szBuf[4096] = {};

	static std::string svOut;
	static std::string svAnsiOut;

	static std::regex rxAnsiExp("\\\033\\[.*?m");

	static std::shared_ptr<spdlog::logger> iconsole = spdlog::get("game_console");
	static std::shared_ptr<spdlog::logger> wconsole = spdlog::get("win_console");
	static std::shared_ptr<spdlog::logger> sqlogger = spdlog::get("sdk_warn");

	s_LogMutex.lock();
	{/////////////////////////////
		va_list args{};
		va_start(args, fmt);

		vsnprintf(szBuf, sizeof(szBuf), fmt, args);

		szBuf[sizeof(szBuf) - 1] = 0;
		va_end(args);
	}/////////////////////////////

	svOut = Plat_GetProcessUpTime();
	svOut.append(sDLL_T[static_cast<int>(context)]);
	svOut.append(szBuf);
	svOut = std::regex_replace(svOut, rxAnsiExp, "");

	if (svOut.back() != '\n')
	{
		svOut.append("\n");
	}

	if (!g_bSpdLog_UseAnsiClr)
	{
		wconsole->debug(svOut);
#ifdef DEDICATED
		RCONServer()->Send(RCONServer()->Serialize(svOut, "", sv_rcon::response_t::SERVERDATA_RESPONSE_CONSOLE_LOG));
#endif // DEDICATED
	}
	else
	{
		svAnsiOut = Plat_GetProcessUpTime();
		svAnsiOut.append(sANSI_DLL_T[static_cast<int>(context)]);
		svAnsiOut.append(g_svYellowF);
		svAnsiOut.append(szBuf);

		if (svAnsiOut.back() != '\n')
		{
			svAnsiOut.append("\n");
		}
		wconsole->debug(svAnsiOut);
#ifdef DEDICATED
		RCONServer()->Send(RCONServer()->Serialize(svAnsiOut, "", sv_rcon::response_t::SERVERDATA_RESPONSE_CONSOLE_LOG));
#endif // DEDICATED
	}

	sqlogger->debug(svOut);

#ifndef DEDICATED
	iconsole->info(svOut);

	g_pConsole->AddLog(ConLog_t(g_spd_sys_w_oss.str(), ImVec4(1.00f, 1.00f, 0.00f, 0.80f)));
	g_pLogSystem.AddLog(LogType_t::WARNING_C, g_spd_sys_w_oss.str());

	g_spd_sys_w_oss.str("");
	g_spd_sys_w_oss.clear();
#endif // !DEDICATED
	s_LogMutex.unlock();
}

//-----------------------------------------------------------------------------
// Purpose: Print engine and SDK errors
// Input  : idx - 
//			*fmt - ... - 
//-----------------------------------------------------------------------------
void Error(eDLL_T context, const char* fmt, ...)
{
	static char szBuf[4096] = {};

	static std::string svOut;
	static std::string svAnsiOut;

	static std::regex rxAnsiExp("\\\033\\[.*?m");

	static std::shared_ptr<spdlog::logger> iconsole = spdlog::get("game_console");
	static std::shared_ptr<spdlog::logger> wconsole = spdlog::get("win_console");
	static std::shared_ptr<spdlog::logger> sqlogger = spdlog::get("sdk_error");

	s_LogMutex.lock();
	{/////////////////////////////
		va_list args{};
		va_start(args, fmt);

		vsnprintf(szBuf, sizeof(szBuf), fmt, args);

		szBuf[sizeof(szBuf) - 1] = 0;
		va_end(args);
	}/////////////////////////////

	svOut = Plat_GetProcessUpTime();
	svOut.append(sDLL_T[static_cast<int>(context)]);
	svOut.append(szBuf);
	svOut = std::regex_replace(svOut, rxAnsiExp, "");

	if (svOut.back() != '\n')
	{
		svOut.append("\n");
	}

	if (!g_bSpdLog_UseAnsiClr)
	{
		wconsole->debug(svOut);
#ifdef DEDICATED
		RCONServer()->Send(RCONServer()->Serialize(svOut, "", sv_rcon::response_t::SERVERDATA_RESPONSE_CONSOLE_LOG));
#endif // DEDICATED
	}
	else
	{
		svAnsiOut = Plat_GetProcessUpTime();
		svAnsiOut.append(sANSI_DLL_T[static_cast<int>(context)]);
		svAnsiOut.append(g_svRedF);
		svAnsiOut.append(szBuf);

		if (svAnsiOut.back() != '\n')
		{
			svAnsiOut.append("\n");
		}
		wconsole->debug(svAnsiOut);
#ifdef DEDICATED
		RCONServer()->Send(RCONServer()->Serialize(svAnsiOut, "", sv_rcon::response_t::SERVERDATA_RESPONSE_CONSOLE_LOG));
#endif // DEDICATED
	}

	sqlogger->debug(svOut);

#ifndef DEDICATED
	iconsole->info(svOut);

	g_pConsole->AddLog(ConLog_t(g_spd_sys_w_oss.str(), ImVec4(1.00f, 0.00f, 0.00f, 1.00f)));
	g_pLogSystem.AddLog(LogType_t::ERROR_C, g_spd_sys_w_oss.str());

	g_spd_sys_w_oss.str("");
	g_spd_sys_w_oss.clear();
#endif // !DEDICATED
	s_LogMutex.unlock();
}