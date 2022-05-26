#include "core/stdafx.h"
#include "engine/sys_dll.h"

//-----------------------------------------------------------------------------
//	Sys_Error_Internal
//
//-----------------------------------------------------------------------------
int HSys_Error_Internal(char* fmt, va_list args)
{
	char buffer[2048]{};
	Error(eDLL_T::NONE, "_______________________________________________________________\n");
	Error(eDLL_T::NONE, "] ENGINE ERROR ################################################\n");
	vsprintf(buffer, fmt, args);
	Error(eDLL_T::NONE, "%s\n", buffer);

	///////////////////////////////////////////////////////////////////////////
	return Sys_Error_Internal(fmt, args);
}

void SysDll_Attach()
{
	DetourAttach((LPVOID*)&Sys_Error_Internal, &HSys_Error_Internal);
}

void SysDll_Detach()
{
	DetourDetach((LPVOID*)&Sys_Error_Internal, &HSys_Error_Internal);
}
