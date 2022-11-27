#include "core/stdafx.h"
#include "milessdk/shared/core.h"

void AIL_LogFunc(int64_t nLogLevel, const char* pszMessage)
{
	DevMsg(eDLL_T::AUDIO, pszMessage);
	v_AIL_LogFunc(nLogLevel, pszMessage);
}

void MilesCore_Attach()
{
	DetourAttach(&v_AIL_LogFunc, &AIL_LogFunc);
}

void MilesCore_Detach()
{
	DetourDetach(&v_AIL_LogFunc, &AIL_LogFunc);
}