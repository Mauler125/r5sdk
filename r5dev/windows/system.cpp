#include "core/stdafx.h"
#include "windows/system.h"

///////////////////////////////////////////////////////////////////////////////
typedef BOOL(WINAPI* IGetVersionExA)(_Inout_ LPOSVERSIONINFOA lpVersionInformation);
static IGetVersionExA                                    g_oGetVersionExA = nullptr;

//#############################################################################
// SYSTEM HOOKS
//#############################################################################

BOOL WINAPI HGetVersionExA(_Inout_ LPOSVERSIONINFOA lpVersionInformation)
{
#ifdef DEDICATED
	// Return false for dedicated to skip 'SetPRocessDpiAwareness' in 'CEngineAPI:OnStartup()'.
	return NULL;
#else
	return g_oGetVersionExA(lpVersionInformation);
#endif // DEDICATED
}

//#############################################################################
// MANAGEMENT
//#############################################################################

void WinSys_Init()
{
	g_oGetVersionExA = (IGetVersionExA)DetourFindFunction("KERNEL32.dll", "GetVersionExA");
}

void WinSys_Attach()
{
	WinSys_Init();
	///////////////////////////////////////////////////////////////////////////
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	///////////////////////////////////////////////////////////////////////////
	DetourAttach(&(LPVOID&)g_oGetVersionExA, (PBYTE)HGetVersionExA);

	///////////////////////////////////////////////////////////////////////////
	DetourTransactionCommit();
}

void WinSys_Detach()
{
	///////////////////////////////////////////////////////////////////////////
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	///////////////////////////////////////////////////////////////////////////
	DetourDetach(&(LPVOID&)g_oGetVersionExA, (PBYTE)HGetVersionExA);

	///////////////////////////////////////////////////////////////////////////
	DetourTransactionCommit();
}
