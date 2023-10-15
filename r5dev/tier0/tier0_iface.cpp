//===========================================================================//
//
// Purpose: Low-level tier0 interface.
//
//===========================================================================//
#include "core/logdef.h"
#include "common/sdkdefs.h"
#include "tier0/module.h"
// Module handles; user is responsible for initializing these.

CModule g_GameDll;
CModule g_SDKDll;

CModule g_RadVideoToolsDll;
CModule g_RadAudioDecoderDll;
CModule g_RadAudioSystemDll;

string g_LogSessionUUID;
string g_LogSessionDirectory;

static const char* const s_AdrFmt = "| {:s}: {:42s}: {:#18x} |\n";

void LogFunAdr(const char* const szFun, const uintptr_t nAdr) // Logging function addresses.
{
	if (!IsCert() && !IsRetail())
		spdlog::debug(s_AdrFmt, "FUN", szFun, nAdr);
	else
	{
		NOTE_UNUSED(szFun);
		NOTE_UNUSED(nAdr);
	}
}
void LogVarAdr(const char* const szVar, const uintptr_t nAdr) // Logging variable addresses.
{
	if (!IsCert() && !IsRetail())
		spdlog::debug(s_AdrFmt, "VAR", szVar, nAdr);
	else
	{
		NOTE_UNUSED(szVar);
		NOTE_UNUSED(nAdr);
	}
}
void LogConAdr(const char* const szCon, const uintptr_t nAdr) // Logging constant addresses.
{
	if (!IsCert() && !IsRetail())
		spdlog::debug(s_AdrFmt, "CON", szCon, nAdr);
	else
	{
		NOTE_UNUSED(szCon);
		NOTE_UNUSED(nAdr);
	}
}
