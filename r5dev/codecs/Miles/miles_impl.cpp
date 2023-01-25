#include "core/stdafx.h"
#include "miles_impl.h"
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
	DevMsg(eDLL_T::AUDIO, __FUNCTION__": initializing Miles Sound System\n");

	bool bResult = v_Miles_Initialize();

	bResult	? DevMsg(eDLL_T::AUDIO, __FUNCTION__": %s\n", "initialized successfully")
			: Warning(eDLL_T::AUDIO, __FUNCTION__": %s\n", "failed to initialize");

	return bResult;
}

void MilesQueueEventRun(Miles::Queue* queue, const char* eventName)
{
	if(miles_debug->GetBool())
		DevMsg(eDLL_T::AUDIO, __FUNCTION__": running event '%s'\n", eventName);

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