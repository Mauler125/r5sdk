#include <core/stdafx.h>
#include <core/init.h>
#include <launcher/prx.h>

//-----------------------------------------------------------------------------
// Purpose: shutdown and unload SDK
//-----------------------------------------------------------------------------
void h_exit_or_terminate_process(UINT uExitCode)
{
	R5Dev_Shutdown();

	HANDLE h = GetCurrentProcess();
	TerminateProcess(h, uExitCode);
}

void PRX_Attach()
{
	DetourAttach((LPVOID*)&v_exit_or_terminate_process, &h_exit_or_terminate_process);
}

void PRX_Detach()
{
	DetourDetach((LPVOID*)&v_exit_or_terminate_process, &h_exit_or_terminate_process);
}