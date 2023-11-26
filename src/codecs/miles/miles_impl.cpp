#include "core/stdafx.h"
#include "miles_impl.h"
#include "tier0/fasttimer.h"
#include "tier1/cvar.h"

//-----------------------------------------------------------------------------
// Purpose: logs debug output emitted from the Miles Sound System
// Input  : nLogLevel - 
//          pszMessage - 
//-----------------------------------------------------------------------------
void AIL_LogFunc(int64_t nLogLevel, const char* pszMessage)
{
	Msg(eDLL_T::AUDIO, "%s\n", pszMessage);
	v_AIL_LogFunc(nLogLevel, pszMessage);
}

//-----------------------------------------------------------------------------
// Purpose: initializes the miles sound system
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool Miles_Initialize()
{
	const char* pszLanguage = miles_language->GetString();
	if (!pszLanguage[0])
	{
		pszLanguage = MILES_DEFAULT_LANGUAGE;
	}

	Msg(eDLL_T::AUDIO, "%s: initializing MSS with language: '%s'\n", __FUNCTION__, pszLanguage);
	CFastTimer initTimer;

	initTimer.Start();
	bool bResult = v_Miles_Initialize();
	initTimer.End();

	Msg(eDLL_T::AUDIO, "%s: %s ('%f' seconds)\n", __FUNCTION__, bResult ? "success" : "failure", initTimer.GetDuration().GetSeconds());
	return bResult;
}

void MilesQueueEventRun(Miles::Queue* queue, const char* eventName)
{
	if(miles_debug->GetBool())
		Msg(eDLL_T::AUDIO, "%s: running event: '%s'\n", __FUNCTION__, eventName);

	v_MilesQueueEventRun(queue, eventName);
}

void MilesBankPatch(Miles::Bank* bank, char* streamPatch, char* localizedStreamPatch)
{
	// TODO [REXX]: add print for patch loading when Miles::Bank struct is mapped out a bit better with file name
	v_MilesBankPatch(bank, streamPatch, localizedStreamPatch);
}

///////////////////////////////////////////////////////////////////////////////
void MilesCore::Detour(const bool bAttach) const
{
	DetourSetup(&v_AIL_LogFunc, &AIL_LogFunc, bAttach);
	DetourSetup(&v_Miles_Initialize, &Miles_Initialize, bAttach);
	DetourSetup(&v_MilesQueueEventRun, &MilesQueueEventRun, bAttach);
	DetourSetup(&v_MilesBankPatch, &MilesBankPatch, bAttach);
}
