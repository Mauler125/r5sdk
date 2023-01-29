//===========================================================================//
//
// Purpose: stub implementation of 'tier0/dbg.cpp'
//
//===========================================================================//

#include "core/stdafx.h"

void LogStub(const char* fmt, ...)
{
	static char szBuf[4096] = {};

	{/////////////////////////////
		va_list args{};
		va_start(args, fmt);

		vsnprintf(szBuf, sizeof(szBuf), fmt, args);

		szBuf[sizeof(szBuf) - 1] = '\0';
		va_end(args);
	}/////////////////////////////

	printf("%s", szBuf);
}

//-----------------------------------------------------------------------------
// Purpose: Netconsole log
// Input  : context - 
//			*fmt - ... - 
//-----------------------------------------------------------------------------
void NetMsg(EGlobalContext_t context, const char* fmt, ...)
{
	static char szBuf[4096] = {};

	{/////////////////////////////
		va_list args{};
		va_start(args, fmt);

		vsnprintf(szBuf, sizeof(szBuf), fmt, args);

		szBuf[sizeof(szBuf) - 1] = '\0';
		va_end(args);
	}/////////////////////////////

	printf("%s", szBuf);
}

//-----------------------------------------------------------------------------
// Purpose: Show logs to all console interfaces
// Input  : context - 
//			*fmt - ... - 
//-----------------------------------------------------------------------------
void DevMsg(eDLL_T context, const char* fmt, ...)
{
	static char szBuf[4096] = {};

	{/////////////////////////////
		va_list args{};
		va_start(args, fmt);

		vsnprintf(szBuf, sizeof(szBuf), fmt, args);

		szBuf[sizeof(szBuf) - 1] = '\0';
		va_end(args);
	}/////////////////////////////

	printf("%s", szBuf);
}

//-----------------------------------------------------------------------------
// Purpose: Print engine and SDK errors
// Input  : context - 
//			*fmt - ... - 
//-----------------------------------------------------------------------------
void Warning(eDLL_T context, const char* fmt, ...)
{
	static char szBuf[4096] = {};

	{/////////////////////////////
		va_list args{};
		va_start(args, fmt);

		vsnprintf(szBuf, sizeof(szBuf), fmt, args);

		szBuf[sizeof(szBuf) - 1] = '\0';
		va_end(args);
	}/////////////////////////////

	printf("%s", szBuf);
}

//-----------------------------------------------------------------------------
// Purpose: Print engine and SDK errors
// Input  : context - 
//			code - 
//			*fmt - ... - 
//-----------------------------------------------------------------------------
void Error(eDLL_T context, const UINT code, const char* fmt, ...)
{
	static char szBuf[4096] = {};

	{/////////////////////////////
		va_list args{};
		va_start(args, fmt);

		vsnprintf(szBuf, sizeof(szBuf), fmt, args);

		szBuf[sizeof(szBuf) - 1] = '\0';
		va_end(args);
	}/////////////////////////////

	printf("%s", szBuf);
	if (code) // Terminate the process if an exit code was passed.
	{
		if (MessageBoxA(NULL, szBuf, "SDK Error", MB_ICONERROR | MB_OK))
		{
			TerminateProcess(GetCurrentProcess(), code);
		}
	}
}
