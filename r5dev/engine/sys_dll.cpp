#include "core/stdafx.h"
#include "engine/sys_dll.h"

//-----------------------------------------------------------------------------
//	Sys_Error_Internal
//
//-----------------------------------------------------------------------------
int HSys_Error_Internal(char* fmt, va_list args)
{
	char buffer[2048]{};
	Error(eDLL_T::COMMON, NO_ERROR, "_______________________________________________________________\n");
	Error(eDLL_T::COMMON, NO_ERROR, "] ENGINE ERROR ################################################\n");
	vsprintf(buffer, fmt, args);
	Error(eDLL_T::COMMON, NO_ERROR, "%s\n", buffer);

	///////////////////////////////////////////////////////////////////////////
	return Sys_Error_Internal(fmt, args);
}

void VSys_Dll::Attach() const
{
	DetourAttach(&Sys_Error_Internal, &HSys_Error_Internal);
}

void VSys_Dll::Detach() const
{
	DetourDetach(&Sys_Error_Internal, &HSys_Error_Internal);
}
