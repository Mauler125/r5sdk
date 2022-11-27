#include "core/stdafx.h"
#include "milessdk/shared/core.h"

void AIL_LogFunc(int64_t nLogLevel, const char* pszMessage)
{
	DevMsg(eDLL_T::AUDIO, pszMessage);
	v_AIL_LogFunc(nLogLevel, pszMessage);
}

bool Miles_Initialize()
{
	bool result = v_Miles_Initialize();

	DevMsg(eDLL_T::AUDIO, "Miles_Initialize: %s\n", result ? "initialized successfully" : "failed to initialize");

	return result;
}

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