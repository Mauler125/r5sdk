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
#ifndef NETCONSOLE
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
#include "squirrel/sqstdaux.h"
#endif // !NETCONSOLE
std::mutex g_LogMutex;

//-----------------------------------------------------------------------------
// True if -hushasserts was passed on command line.
//-----------------------------------------------------------------------------
bool HushAsserts()
{
#if defined (DBGFLAG_ASSERT) && !defined (NETCONSOLE)
	static bool s_bHushAsserts = !!CommandLine()->FindParm("-hushasserts");
	return s_bHushAsserts;
#else
	return true;
#endif
}

//-----------------------------------------------------------------------------
// Templates to assist in validating pointers:
//-----------------------------------------------------------------------------
/*PLATFORM_INTERFACE*/ void _AssertValidReadPtr(void* ptr, int count/* = 1*/)
{
#if defined( _WIN32 ) && !defined( _X360 )
	Assert(!IsBadReadPtr(ptr, count));
#else
	Assert(!count || ptr);
#endif
#ifdef NDEBUG
	NOTE_UNUSED(ptr);
	NOTE_UNUSED(count);
#endif // NDEBUG
}

/*PLATFORM_INTERFACE*/ void _AssertValidWritePtr(void* ptr, int count/* = 1*/)
{
#if defined( _WIN32 ) && !defined( _X360 )
	Assert(!IsBadWritePtr(ptr, count));
#else
	Assert(!count || ptr);
#endif
#ifdef NDEBUG
	NOTE_UNUSED(ptr);
	NOTE_UNUSED(count);
#endif // NDEBUG
}

/*PLATFORM_INTERFACE*/ void _AssertValidReadWritePtr(void* ptr, int count/* = 1*/)
{
#if defined( _WIN32 ) && !defined( _X360 )
	Assert(!(IsBadWritePtr(ptr, count) || IsBadReadPtr(ptr, count)));
#else
	Assert(!count || ptr);
#endif
#ifdef NDEBUG
	NOTE_UNUSED(ptr);
	NOTE_UNUSED(count);
#endif // NDEBUG
}

/*PLATFORM_INTERFACE*/ void _AssertValidStringPtr(const TCHAR* ptr, int maxchar/* = 0xFFFFFF */)
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
#ifdef NDEBUG
	NOTE_UNUSED(ptr);
	NOTE_UNUSED(maxchar);
#endif // NDEBUG
}

/*PLATFORM_INTERFACE*/ void AssertValidWStringPtr(const wchar_t* ptr, int maxchar/* = 0xFFFFFF */)
{
#if defined( _WIN32 ) && !defined( _X360 )
	Assert(!IsBadStringPtrW(ptr, maxchar));
#else
	Assert(ptr);
#endif
#ifdef NDEBUG
	NOTE_UNUSED(ptr);
	NOTE_UNUSED(maxchar);
#endif // NDEBUG
}

#if !defined (DEDICATED) && !defined (NETCONSOLE)
ImVec4 CheckForWarnings(LogType_t type, eDLL_T context, const ImVec4& defaultCol)
{
	ImVec4 color = defaultCol;
	if (type == LogType_t::LOG_WARNING || context == eDLL_T::SYSTEM_WARNING)
	{
		color = ImVec4(1.00f, 1.00f, 0.00f, 0.80f);
	}
	else if (type == LogType_t::LOG_ERROR || context == eDLL_T::SYSTEM_ERROR)
	{
		color = ImVec4(1.00f, 0.00f, 0.00f, 0.80f);
	}

	return color;
}

ImVec4 GetColorForContext(LogType_t type, eDLL_T context)
{
	switch (context)
	{
	case eDLL_T::SCRIPT_SERVER:
		return CheckForWarnings(type, context, ImVec4(0.59f, 0.58f, 0.73f, 1.00f));
	case eDLL_T::SCRIPT_CLIENT:
		return CheckForWarnings(type, context, ImVec4(0.59f, 0.58f, 0.63f, 1.00f));
	case eDLL_T::SCRIPT_UI:
		return CheckForWarnings(type, context, ImVec4(0.59f, 0.48f, 0.53f, 1.00f));
	case eDLL_T::SERVER:
		return CheckForWarnings(type, context, ImVec4(0.23f, 0.47f, 0.85f, 1.00f));
	case eDLL_T::CLIENT:
		return CheckForWarnings(type, context, ImVec4(0.46f, 0.46f, 0.46f, 1.00f));
	case eDLL_T::UI:
		return CheckForWarnings(type, context, ImVec4(0.59f, 0.35f, 0.46f, 1.00f));
	case eDLL_T::ENGINE:
		return CheckForWarnings(type, context, ImVec4(0.70f, 0.70f, 0.70f, 1.00f));
	case eDLL_T::FS:
		return CheckForWarnings(type, context, ImVec4(0.32f, 0.64f, 0.72f, 1.00f));
	case eDLL_T::RTECH:
		return CheckForWarnings(type, context, ImVec4(0.36f, 0.70f, 0.35f, 1.00f));
	case eDLL_T::MS:
		return CheckForWarnings(type, context, ImVec4(0.75f, 0.30f, 0.68f, 1.00f));
	case eDLL_T::AUDIO:
		return CheckForWarnings(type, context, ImVec4(0.93f, 0.42f, 0.12f, 1.00f));
	case eDLL_T::VIDEO:
		return CheckForWarnings(type, context, ImVec4(0.73f, 0.00f, 0.92f, 1.00f));
	case eDLL_T::NETCON:
		return CheckForWarnings(type, context, ImVec4(0.81f, 0.81f, 0.81f, 1.00f));
	case eDLL_T::COMMON:
		return CheckForWarnings(type, context, ImVec4(1.00f, 0.80f, 0.60f, 1.00f));
	default:
		return CheckForWarnings(type, context, ImVec4(0.81f, 0.81f, 0.81f, 1.00f));
	}
}
#endif // !DEDICATED && !NETCONSOLE

const char* GetContextNameByIndex(eDLL_T context, const bool ansiColor = false)
{
	int index = static_cast<int>(context);
	const char* contextName = s_DefaultAnsiColor;

	switch (context)
	{
	case eDLL_T::SCRIPT_SERVER:
		contextName = s_ScriptAnsiColor[0];
		break;
	case eDLL_T::SCRIPT_CLIENT:
		contextName = s_ScriptAnsiColor[1];
		break;
	case eDLL_T::SCRIPT_UI:
		contextName = s_ScriptAnsiColor[2];
		break;
	case eDLL_T::SERVER:
	case eDLL_T::CLIENT:
	case eDLL_T::UI:
	case eDLL_T::ENGINE:
	case eDLL_T::FS:
	case eDLL_T::RTECH:
	case eDLL_T::MS:
	case eDLL_T::AUDIO:
	case eDLL_T::VIDEO:
	case eDLL_T::NETCON:
	case eDLL_T::COMMON:
		contextName = s_DllAnsiColor[index];
		break;
	case eDLL_T::SYSTEM_WARNING:
	case eDLL_T::SYSTEM_ERROR:
	case eDLL_T::NONE:
	default:
		break;
	}

	if (!ansiColor)
	{
		// Shift # chars to skip ANSI row.
		contextName += sizeof(s_DefaultAnsiColor)-1;
	}

	return contextName;
}

//-----------------------------------------------------------------------------
// Purpose: Show logs to all console interfaces (va_list version)
// Input  : logType - 
//			logLevel - 
//			context - 
//			*pszLogger - 
//			*pszFormat -
//			args - 
//			exitCode - 
//			*pszUptimeOverride - 
//-----------------------------------------------------------------------------
void CoreMsgV(LogType_t logType, LogLevel_t logLevel, eDLL_T context,
	const char* pszLogger, const char* pszFormat, va_list args,
	const UINT exitCode /*= NO_ERROR*/, const char* pszUptimeOverride /*= nullptr*/)
{
	const char* pszUpTime = pszUptimeOverride ? pszUptimeOverride : Plat_GetProcessUpTime();
	string message = g_bSpdLog_PostInit ? pszUpTime : "";

	const bool bToConsole = (logLevel >= LogLevel_t::LEVEL_CONSOLE);
	const bool bUseColor  = (bToConsole && g_bSpdLog_UseAnsiClr);

	const char* pszContext = GetContextNameByIndex(context, bUseColor);
	message.append(pszContext);

#if !defined (DEDICATED) && !defined (NETCONSOLE)
	ImVec4 overlayColor   = GetColorForContext(logType, context);
	eDLL_T overlayContext = context;
#endif // !DEDICATED && !NETCONSOLE

	bool bSquirrel = false;
#if !defined (NETCONSOLE)
	bool bWarning  = false;
	bool bError    = false;
#endif // !NETCONSOLE

	//-------------------------------------------------------------------------
	// Setup logger and context
	//-------------------------------------------------------------------------
	switch (logType)
	{
	case LogType_t::LOG_WARNING:
#if !defined (DEDICATED) && !defined (NETCONSOLE)
		overlayContext = eDLL_T::SYSTEM_WARNING;
#endif // !DEDICATED && !NETCONSOLE
		if (bUseColor)
		{
			message.append(g_svYellowF);
		}
		break;
	case LogType_t::LOG_ERROR:
#if !defined (DEDICATED) && !defined (NETCONSOLE)
		overlayContext = eDLL_T::SYSTEM_ERROR;
#endif // !DEDICATED && !NETCONSOLE
		if (bUseColor)
		{
			message.append(g_svRedF);
		}
		break;
#ifndef NETCONSOLE
	case LogType_t::SQ_INFO:
		bSquirrel = true;
		break;
	case LogType_t::SQ_WARNING:
#ifndef DEDICATED
		overlayContext = eDLL_T::SYSTEM_WARNING;
		overlayColor = ImVec4(1.00f, 1.00f, 0.00f, 0.80f);
#endif // !DEDICATED
		bSquirrel = true;
		bWarning = true;
		break;
#endif // !NETCONSOLE
	}

	//-------------------------------------------------------------------------
	// Format actual input
	//-------------------------------------------------------------------------
	va_list argsCopy;
	va_copy(argsCopy, args);
	const string formatted = FormatV(pszFormat, argsCopy);
	va_end(argsCopy);

#ifndef NETCONSOLE
	if (bUseColor && bSquirrel)
	{
		if (bWarning && g_bSQAuxError)
		{
			if (formatted.find("SCRIPT ERROR:") != string::npos ||
				formatted.find(" -> ") != string::npos)
			{
				bError = true;
			}
		}
		else if (g_bSQAuxBadLogic)
		{
			if (formatted.find("There was a problem processing game logic.") != string::npos)
			{
				bError = true;
				g_bSQAuxBadLogic = false;
			}
		}

		// Append warning/error color before appending the formatted text,
		// so that this gets marked as such while preserving context colors.
		if (bError)
		{
#ifndef DEDICATED
			overlayContext = eDLL_T::SYSTEM_ERROR;
			overlayColor = ImVec4(1.00f, 0.00f, 0.00f, 0.80f);
#endif // !DEDICATED
			message.append(g_svRedF);
		}
		else if (bWarning)
		{
			message.append(g_svYellowF);
		}
	}
#endif // !NETCONSOLE
	message.append(formatted);

	//-------------------------------------------------------------------------
	// Emit to all interfaces
	//-------------------------------------------------------------------------
	std::lock_guard<std::mutex> lock(g_LogMutex);
	if (bToConsole)
	{
		g_TermLogger->debug(message);

		if (bUseColor)
		{
			// Remove ANSI rows before emitting to file or over wire.
			message = std::regex_replace(message, s_AnsiRowRegex, "");
		}
	}

#ifndef NETCONSOLE
	// Output is always logged to the file.
	std::shared_ptr<spdlog::logger> ntlogger = spdlog::get(pszLogger); // <-- Obtain by 'pszLogger'.
	assert(ntlogger.get() != nullptr);
	ntlogger->debug(message);

	if (bToConsole)
	{
#ifndef CLIENT_DLL
		RCONServer()->Send(formatted, pszUpTime, sv_rcon::response_t::SERVERDATA_RESPONSE_CONSOLE_LOG,
			static_cast<int>(context), static_cast<int>(logType));
#endif // !CLIENT_DLL
#ifndef DEDICATED
		g_ImGuiLogger->debug(message);

		if (g_bSpdLog_PostInit)
		{
			g_pConsole->AddLog(ConLog_t(g_LogStream.str(), overlayColor));

			if (logLevel >= LogLevel_t::LEVEL_NOTIFY) // Draw to mini console.
			{
				g_pOverlay->AddLog(overlayContext, g_LogStream.str());
			}
		}
#endif // !DEDICATED
	}

#ifndef DEDICATED
	g_LogStream.str(string());
	g_LogStream.clear();
#endif // !DEDICATED

#endif // !NETCONSOLE

	if (exitCode) // Terminate the process if an exit code was passed.
	{
		if (MessageBoxA(NULL, Format("%s- %s", pszUpTime, message.c_str()).c_str(),
			"SDK Error", MB_ICONERROR | MB_OK))
		{
			TerminateProcess(GetCurrentProcess(), exitCode);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Show logs to all console interfaces
// Input  : logType - 
//			logLevel - 
//			context - 
//			exitCode - 
//			*pszLogger - 
//			*pszFormat -
//			... - 
//-----------------------------------------------------------------------------
void CoreMsg(LogType_t logType, LogLevel_t logLevel, eDLL_T context,
	const UINT exitCode, const char* pszLogger, const char* pszFormat, ...)
{
	va_list args;
	va_start(args, pszFormat);
	CoreMsgV(logType, logLevel, context, pszLogger, pszFormat, args, exitCode);
	va_end(args);
}

//-----------------------------------------------------------------------------
// Purpose: Prints general debugging messages
// Input  : context - 
//			*fmt - ... - 
//-----------------------------------------------------------------------------
void DevMsg(eDLL_T context, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	CoreMsgV(LogType_t::LOG_INFO, LogLevel_t::LEVEL_NOTIFY, context, "sdk", fmt, args);
	va_end(args);
}

//-----------------------------------------------------------------------------
// Purpose: Prints logs from remote console
// Input  : context - 
//			*fmt - ... - 
//-----------------------------------------------------------------------------
void NetMsg(LogType_t logType, eDLL_T context, const char* uptime, const char* fmt, ...)
{
#ifndef DEDICATED
	va_list args;
	va_start(args, fmt);
	CoreMsgV(logType, LogLevel_t::LEVEL_NOTIFY, context, "netconsole", fmt, args, NO_ERROR, uptime);
	va_end(args);
#endif // !DEDICATED
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
	CoreMsgV(LogType_t::LOG_WARNING, LogLevel_t::LEVEL_NOTIFY, context, "sdk(warning)", fmt, args);
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
	CoreMsgV(LogType_t::LOG_ERROR, LogLevel_t::LEVEL_NOTIFY, context, "sdk(error)", fmt, args, code);
	va_end(args);
}