#include "core/stdafx.h"
#include "engine/sys_dll.h"

//-----------------------------------------------------------------------------
//	Sys_Error_Internal
//
//-----------------------------------------------------------------------------
int HSys_Error_Internal(char* fmt, va_list args)
{
	printf("\n______________________________________________________________\n");
	printf("] ENGINE ERROR ################################################\n");
	vprintf(fmt, args);

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
