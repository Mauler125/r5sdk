#include <core/stdafx.h>
#include <core/init.h>
#include <launcher/prx.h>

//-----------------------------------------------------------------------------
// Purpose: shutdown and unload SDK
//-----------------------------------------------------------------------------
void h_exit_or_terminate_process(UINT uExitCode)
{
	//SDK_Shutdown();

	HANDLE h = GetCurrentProcess();
	TerminateProcess(h, uExitCode);
}

void VPRX::Detour(const bool bAttach) const
{
	//DetourSetup(&v_exit_or_terminate_process, &h_exit_or_terminate_process, bAttach);
}
