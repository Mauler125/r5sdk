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
#include "tier0/commandline.h"
#ifndef DEDICATED
#include "vgui/vgui_debugpanel.h"
#include "gameui/IConsole.h"
#endif // !DEDICATED
#ifndef CLIENT_DLL
#include "engine/server/sv_rcon.h"
#endif // !CLIENT_DLL

#if defined( _X360 )
#include "xbox/xbox_console.h"
#endif
std::mutex g_LogMutex;

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
// Purpose: Netconsole log
// Input  : context - 
//			*fmt - ... - 
//-----------------------------------------------------------------------------
void NetMsg(EGlobalContext_t context, const char* fmt, ...)
{
#ifndef DEDICATED
	static char szBuf[4096] = {};
	static std::string svOut;

	static const std::shared_ptr<spdlog::logger> iconsole = spdlog::get("game_console");
	static const std::shared_ptr<spdlog::logger> wconsole = spdlog::get("win_console");
	static const std::shared_ptr<spdlog::logger> ntlogger = spdlog::get("net_con");
	switch (context)
	{
	case EGlobalContext_t::GLOBAL_NONE:
	case EGlobalContext_t::SCRIPT_SERVER:
	case EGlobalContext_t::SCRIPT_CLIENT:
	case EGlobalContext_t::SCRIPT_UI:
	{
		g_LogMutex.lock();
		{/////////////////////////////
			va_list args{};
			va_start(args, fmt);

			vsnprintf(szBuf, sizeof(szBuf), fmt, args);

			szBuf[sizeof(szBuf) - 1] = '\0';
			va_end(args);
		}/////////////////////////////

		svOut = szBuf;
		if (svOut.back() != '\n')
		{
			svOut.append("\n");
		}
		ImVec4 color;

		if (svOut.find("\033[38;2;255;255;000m") != std::string::npos)
		{ // Warning.
			color = ImVec4(1.00f, 1.00f, 0.00f, 0.80f);
			context = EGlobalContext_t::WARNING_C;
		}
		else if (svOut.find("\033[38;2;255;000;000m") != std::string::npos)
		{ // Error.
			color = ImVec4(1.00f, 0.00f, 0.00f, 0.80f);
			context = EGlobalContext_t::ERROR_C;
		}
		else
		{
			switch (context)
			{
			case EGlobalContext_t::SCRIPT_SERVER: // [ SERVER ]
				color = ImVec4(0.59f, 0.58f, 0.73f, 1.00f);
				break;
			case EGlobalContext_t::SCRIPT_CLIENT: // [ CLIENT ]
				color = ImVec4(0.59f, 0.58f, 0.63f, 1.00f);
				break;
			case EGlobalContext_t::SCRIPT_UI:     // [   UI   ]
				color = ImVec4(0.59f, 0.48f, 0.53f, 1.00f);
				break;
			default:
				color = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
				break;
			}
		}

		if (g_bSpdLog_UseAnsiClr)
		{
			wconsole->debug(svOut);
			svOut = std::regex_replace(svOut, rANSI_EXP, "");
		}
		else
		{
			svOut = std::regex_replace(svOut, rANSI_EXP, "");
			wconsole->debug(svOut);
		}

		ntlogger->debug(svOut);
		iconsole->debug(svOut);

		g_pConsole->AddLog(ConLog_t(g_LogStream.str(), color));
		g_pOverlay->AddLog(static_cast<EGlobalContext_t>(context), g_LogStream.str());

		g_LogStream.str("");
		g_LogStream.clear();

		g_LogMutex.unlock();

		break;
	}
	case EGlobalContext_t::NATIVE_SERVER:
	case EGlobalContext_t::NATIVE_CLIENT:
	case EGlobalContext_t::NATIVE_UI:
	case EGlobalContext_t::NATIVE_ENGINE:
	case EGlobalContext_t::NATIVE_FS:
	case EGlobalContext_t::NATIVE_RTECH:
	case EGlobalContext_t::NATIVE_MS:
	case EGlobalContext_t::NATIVE_AUDIO:
	case EGlobalContext_t::NATIVE_VIDEO:
	case EGlobalContext_t::NETCON_S:
	case EGlobalContext_t::COMMON_C:
	{
		g_LogMutex.lock();
		{/////////////////////////////
			va_list args{};
			va_start(args, fmt);

			vsnprintf(szBuf, sizeof(szBuf), fmt, args);

			szBuf[sizeof(szBuf) - 1] = '\0';
			va_end(args);
		}/////////////////////////////

		svOut = szBuf;
		if (svOut.back() != '\n')
		{
			svOut.append("\n");
		}
		ImVec4 color;

		if (svOut.find("\033[38;2;255;255;000;") != std::string::npos)
		{ // Warning.
			color = ImVec4(1.00f, 1.00f, 0.00f, 0.80f);
			context = EGlobalContext_t::WARNING_C;
		}
		else if (svOut.find("\033[38;2;255;000;000;") != std::string::npos)
		{ // Error.
			color = ImVec4(1.00f, 0.00f, 0.00f, 0.80f);
			context = EGlobalContext_t::ERROR_C;
		}
		else
		{
			switch (static_cast<eDLL_T>(context))
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
				color = ImVec4(0.75f, 0.30f, 0.68f, 1.00f);
				break;
			case eDLL_T::AUDIO:
				color = ImVec4(0.93f, 0.42f, 0.12f, 1.00f);
				break;
			case eDLL_T::VIDEO:
				color = ImVec4(0.73f, 0.00f, 0.92f, 1.00f);
				break;
			case eDLL_T::NETCON:
				color = ImVec4(0.81f, 0.81f, 0.81f, 1.00f);
				break;
			case eDLL_T::COMMON:
				color = ImVec4(1.00f, 0.80f, 0.60f, 1.00f);
				break;
			default:
				color = ImVec4(0.81f, 0.81f, 0.81f, 1.00f);
				break;
			}
		}

		if (g_bSpdLog_UseAnsiClr)
		{
			wconsole->debug(svOut);
			svOut = std::regex_replace(svOut, rANSI_EXP, "");
		}
		else
		{
			svOut = std::regex_replace(svOut, rANSI_EXP, "");
			wconsole->debug(svOut);
		}

		ntlogger->debug(svOut);
		iconsole->info(svOut);

		g_pConsole->AddLog(ConLog_t(g_LogStream.str(), color));
		g_pOverlay->AddLog(context, g_LogStream.str());

		g_LogStream.str("");
		g_LogStream.clear();
		g_LogMutex.unlock();
		break;
	}
	}
#endif // !DEDICATED
}

#ifndef DEDICATED
ImVec4 GetColorForContext(eDLL_T context)
{
	switch (context)
	{
	case eDLL_T::SERVER:
		return ImVec4(0.23f, 0.47f, 0.85f, 1.00f);
	case eDLL_T::CLIENT:
		return ImVec4(0.46f, 0.46f, 0.46f, 1.00f);
	case eDLL_T::UI:
		return ImVec4(0.59f, 0.35f, 0.46f, 1.00f);
	case eDLL_T::ENGINE:
		return ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
	case eDLL_T::FS:
		return ImVec4(0.32f, 0.64f, 0.72f, 1.00f);
	case eDLL_T::RTECH:
		return ImVec4(0.36f, 0.70f, 0.35f, 1.00f);
	case eDLL_T::MS:
		return ImVec4(0.75f, 0.30f, 0.68f, 1.00f);
	case eDLL_T::AUDIO:
		return ImVec4(0.93f, 0.42f, 0.12f, 1.00f);
	case eDLL_T::VIDEO:
		return ImVec4(0.73f, 0.00f, 0.92f, 1.00f);
	case eDLL_T::NETCON:
		return ImVec4(0.81f, 0.81f, 0.81f, 1.00f);
	case eDLL_T::COMMON:
		return ImVec4(1.00f, 0.80f, 0.60f, 1.00f);
	default:
		return ImVec4(0.81f, 0.81f, 0.81f, 1.00f);
	}
}

#endif // !DEDICATED

string MsgInternal(LogType_t type, eDLL_T context, const char* fmt, va_list args, const UINT code = NULL)
{
	std::lock_guard<std::mutex> lock(g_LogMutex);

	static std::shared_ptr<spdlog::logger> iconsole = spdlog::get("game_console");
	static std::shared_ptr<spdlog::logger> wconsole = spdlog::get("win_console");
	static std::shared_ptr<spdlog::logger> ntlogger; // <-- Obtained by logger context.

	const char* pszUpTime = Plat_GetProcessUpTime();
	string result = pszUpTime;

	const char* pszContext = sANSI_DLL_T[static_cast<int>(context)];
	if (!g_bSpdLog_UseAnsiClr)
	{
		// Shift # chars to skip ansi row.
		pszContext += sizeof(s_DefaultAnsiColor)-1;
	}
	result.append(pszContext);

#ifndef DEDICATED
	ImVec4 color;
	EGlobalContext_t tLog;
#endif // !DEDICATED

	switch (type)
	{
	case LogType_t::LOG_INFO:
#ifndef DEDICATED
		color = GetColorForContext(context);
		tLog = static_cast<EGlobalContext_t>(context);
#endif // !DEDICATED
		ntlogger = spdlog::get("sdk_info");
		break;
	case LogType_t::LOG_WARNING:
#ifndef DEDICATED
		color = ImVec4(1.00f, 1.00f, 0.00f, 0.80f);
		tLog = EGlobalContext_t::WARNING_C;
#endif // !DEDICATED
		ntlogger = spdlog::get("sdk_warn");
		if (g_bSpdLog_UseAnsiClr)
		{
			result.append(g_svYellowF);
		}
		break;
	case LogType_t::LOG_ERROR:
#ifndef DEDICATED
		color = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		tLog = EGlobalContext_t::ERROR_C;
#endif // !DEDICATED
		ntlogger = spdlog::get("sdk_error");
		if (g_bSpdLog_UseAnsiClr)
		{
			result.append(g_svRedF);
		}
		break;
	}

	va_list argsCopy;
	va_copy(argsCopy, args);
	result.append(FormatV(fmt, argsCopy));
	va_end(argsCopy);

	if (result.back() != '\n')
	{
		result.append("\n");
	}

#ifndef CLIENT_DLL
	RCONServer()->Send(result, "", sv_rcon::response_t::SERVERDATA_RESPONSE_CONSOLE_LOG, static_cast<int>(context));
#endif // !CLIENT_DLL

	wconsole->debug(result);
	result = std::regex_replace(result, rANSI_EXP, "");
	ntlogger->debug(result);
#ifndef DEDICATED
	iconsole->debug(result);

	if (g_bSpdLog_PostInit)
	{
		g_pConsole->AddLog(ConLog_t(g_LogStream.str(), color));
		g_pOverlay->AddLog(tLog, g_LogStream.str());
	}

	g_LogStream.str("");
	g_LogStream.clear();
#endif // !DEDICATED

	if (code) // Terminate the process if an exit code was passed.
	{
		if (MessageBoxA(NULL, Format("%s- %s", pszUpTime, result.c_str()).c_str(), "SDK Error", MB_ICONERROR | MB_OK))
		{
			TerminateProcess(GetCurrentProcess(), code);
		}
	}

	return result;
}

//-----------------------------------------------------------------------------
// Purpose: Show logs to all console interfaces
// Input  : context - 
//			*fmt - ... - 
//-----------------------------------------------------------------------------
void DevMsg(eDLL_T context, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	MsgInternal(LogType_t::LOG_INFO, context, fmt, args);
	va_end(args);
}

//-----------------------------------------------------------------------------
// Purpose: Print engine and SDK warnings
// Input  : context - 
//			*fmt - ... - 
//-----------------------------------------------------------------------------
void Warning(eDLL_T context, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	MsgInternal(LogType_t::LOG_WARNING, context, fmt, args);
	va_end(args);
}

//-----------------------------------------------------------------------------
// Purpose: Print engine and SDK errors
// Input  : context - 
//			code - 
//			*fmt - ... - 
//-----------------------------------------------------------------------------
void Error(eDLL_T context, const UINT code, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	MsgInternal(LogType_t::LOG_ERROR, context, fmt, args, code);
	va_end(args);
}