//==== Copyright (c) 1996-2005, Valve Corporation, All rights reserved. =====//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#include "tier0/dbg.h"
#include "tier0/platform.h"
#ifndef NETCONSOLE
#include "tier0/threadtools.h"
#include "tier0/commandline.h"

#if defined( _X360 )
#include "xbox/xbox_console.h"
#endif
#endif // !NETCONSOLE

CoreMsgVCallbackSink_t g_CoreMsgVCallback = nullptr;

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
	Assert(!IsBadStringPtr(ptr, maxchar));
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
	// Must be initialized before calling this function!
	Assert(g_CoreMsgVCallback != nullptr);
	g_CoreMsgVCallback(logType, logLevel, context, pszLogger, pszFormat, args, exitCode, pszUptimeOverride);
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
void Msg(eDLL_T context, const char* fmt, ...)
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
	va_list args;
	va_start(args, fmt);
	CoreMsgV(logType, LogLevel_t::LEVEL_NOTIFY, context, "netconsole", fmt, args, NO_ERROR, uptime);
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
#ifndef DBGFLAG_STRINGS_STRIP
//-----------------------------------------------------------------------------
// Purpose: Prints general debugging messages (uncertain builds only!)
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
// Purpose: Print engine and SDK warnings (uncertain builds only!)
// Input  : context - 
//			*fmt - ... - 
//-----------------------------------------------------------------------------
void DevWarning(eDLL_T context, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	CoreMsgV(LogType_t::LOG_WARNING, LogLevel_t::LEVEL_NOTIFY, context, "sdk(warning)", fmt, args);
	va_end(args);
}
#endif // !DBGFLAG_STRINGS_STRIP
