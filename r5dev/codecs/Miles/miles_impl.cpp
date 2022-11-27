#include "core/stdafx.h"
#include "miles_impl.h"

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
	bool bResult = v_Miles_Initialize();

	bResult	? DevMsg(eDLL_T::AUDIO, __FUNCTION__": %s\n", "initialized successfully")
			: Warning(eDLL_T::AUDIO, __FUNCTION__": %s\n", "failed to initialize");

	return bResult;
}

///////////////////////////////////////////////////////////////////////////////
void MilesCore_Attach()
{
	DetourAttach(&v_AIL_LogFunc, &AIL_LogFunc);
	DetourAttach(&v_Miles_Initialize, &Miles_Initialize);
}

void MilesCore_Detach()
{
	DetourDetach(&v_AIL_LogFunc, &AIL_LogFunc);
	DetourDetach(&v_Miles_Initialize, &Miles_Initialize);
}