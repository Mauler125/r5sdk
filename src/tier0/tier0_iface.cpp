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

void LogFunAdr(const char* const szFun, const void* const pAdr) // Logging function addresses.
{
	if (!IsCert() && !IsRetail())
		spdlog::debug(s_AdrFmt, "FUN", szFun, uintptr_t(pAdr));
	else
	{
		NOTE_UNUSED(szFun);
		NOTE_UNUSED(pAdr);
	}
}
void LogVarAdr(const char* const szVar, const void* const pAdr) // Logging variable addresses.
{
	if (!IsCert() && !IsRetail())
		spdlog::debug(s_AdrFmt, "VAR", szVar, uintptr_t(pAdr));
	else
	{
		NOTE_UNUSED(szVar);
		NOTE_UNUSED(pAdr);
	}
}
void LogConAdr(const char* const szCon, const void* const pAdr) // Logging constant addresses.
{
	if (!IsCert() && !IsRetail())
		spdlog::debug(s_AdrFmt, "CON", szCon, uintptr_t(pAdr));
	else
	{
		NOTE_UNUSED(szCon);
		NOTE_UNUSED(pAdr);
	}
}
