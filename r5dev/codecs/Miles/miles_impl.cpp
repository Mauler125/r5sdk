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
	DevMsg(eDLL_T::AUDIO, pszMessage);
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

	DevMsg(eDLL_T::AUDIO, "%s: initializing MSS with language: '%s'\n", __FUNCTION__, pszLanguage);
	CFastTimer initTimer;

	initTimer.Start();
	bool bResult = v_Miles_Initialize();
	initTimer.End();

	DevMsg(eDLL_T::AUDIO, "%s: %s ('%f' seconds)\n", __FUNCTION__, bResult ? "success" : "failure", initTimer.GetDuration().GetSeconds());
	return bResult;
}

void MilesQueueEventRun(Miles::Queue* queue, const char* eventName)
{
	if(miles_debug->GetBool())
		DevMsg(eDLL_T::AUDIO, "%s: running event: '%s'\n", __FUNCTION__, eventName);

	v_MilesQueueEventRun(queue, eventName);
}

void MilesBankPatch(Miles::Bank* bank, char* streamPatch, char* localizedStreamPatch)
{
	// TODO [REXX]: add print for patch loading when Miles::Bank struct is mapped out a bit better with file name
	v_MilesBankPatch(bank, streamPatch, localizedStreamPatch);
}

///////////////////////////////////////////////////////////////////////////////
void MilesCore::Attach() const
{
	DetourAttach(&v_AIL_LogFunc, &AIL_LogFunc);
	DetourAttach(&v_Miles_Initialize, &Miles_Initialize);
	DetourAttach(&v_MilesQueueEventRun, &MilesQueueEventRun);
	DetourAttach(&v_MilesBankPatch, &MilesBankPatch);
}

void MilesCore::Detach() const
{
	DetourDetach(&v_AIL_LogFunc, &AIL_LogFunc);
	DetourDetach(&v_Miles_Initialize, &Miles_Initialize);
	DetourDetach(&v_MilesQueueEventRun, &MilesQueueEventRun);
	DetourDetach(&v_MilesBankPatch, &MilesBankPatch);
}