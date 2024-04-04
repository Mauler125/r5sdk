#include "core/stdafx.h"
#include "core/init.h"
#include "windows/system.h"
#include "engine/host_state.h"

///////////////////////////////////////////////////////////////////////////////
typedef BOOL(WINAPI* IGetVersionExA)(
	_Inout_ LPOSVERSIONINFOA lpVersionInformation);
typedef BOOL(WINAPI* IPeekMessage)(
	_Out_ LPMSG lpMsg,
	_In_opt_ HWND hWnd,
	_In_ UINT wMsgFilterMin,
	_In_ UINT wMsgFilterMax,
	_In_ UINT wRemoveMsg);
static IGetVersionExA                                    VGetVersionExA = nullptr;
static IPeekMessage                                      VPeekMessageA  = nullptr;
static IPeekMessage                                      VPeekMessageW  = nullptr;

//#############################################################################
// SYSTEM HOOKS
//#############################################################################

BOOL
WINAPI
HGetVersionExA(
	_Inout_ LPOSVERSIONINFOA lpVersionInformation)
{
#ifdef DEDICATED
	// Return false for dedicated to skip 'SetProcessDpiAwareness' in 'CEngineAPI:OnStartup()'.
	return NULL;
#else
	return VGetVersionExA(lpVersionInformation);
#endif // DEDICATED
}

BOOL
WINAPI
HPeekMessage(
	_Out_ LPMSG lpMsg,
	_In_opt_ HWND hWnd,
	_In_ UINT wMsgFilterMin,
	_In_ UINT wMsgFilterMax,
	_In_ UINT wRemoveMsg)
{
#ifdef DEDICATED
	// Return false for dedicated to reduce unnecessary overhead when calling 'PeekMessageA/W()' every frame.
	return NULL;
#else
	return VPeekMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
#endif // DEDICATED
}

BOOL
WINAPI
ConsoleHandlerRoutine(
	DWORD eventCode)
{
	switch (eventCode)
	{
	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		
		if (!g_bSdkShutdownInitiatedFromConsoleHandler)
			g_bSdkShutdownInitiatedFromConsoleHandler = true;

		if (g_pHostState) // This tells the engine to gracefully shutdown on the next frame.
			g_pHostState->m_iNextState = HostStates_t::HS_SHUTDOWN;

		// Give it time to shutdown properly, this loop waits for max time
		// of SPI_GETWAITTOKILLSERVICETIMEOUT, which is 20000ms by default.
		while (g_bSdkInitialized)
			Sleep(50);

		return TRUE;
	}

	return FALSE;
}

//#############################################################################
// MANAGEMENT
//#############################################################################

void WinSys_Init()
{
#ifdef DEDICATED
	VGetVersionExA = (IGetVersionExA)DetourFindFunction("KERNEL32.dll", "GetVersionExA");
	VPeekMessageA = (IPeekMessage)DetourFindFunction("USER32.dll", "PeekMessageA");
	VPeekMessageW = (IPeekMessage)DetourFindFunction("USER32.dll", "PeekMessageW");

	///////////////////////////////////////////////////////////////////////////
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	///////////////////////////////////////////////////////////////////////////
	DetourAttach(&(LPVOID&)VGetVersionExA, (PBYTE)HGetVersionExA);
	DetourAttach(&(LPVOID&)VPeekMessageA, (PBYTE)HPeekMessage);
	//DetourAttach(&(LPVOID&)VPeekMessageW, (PBYTE)HPeekMessage);

	///////////////////////////////////////////////////////////////////////////
	HRESULT hr = DetourTransactionCommit();
	if (hr != NO_ERROR)
	{
		// Failed to hook into the process, terminate
		Assert(0);
		Error(eDLL_T::COMMON, 0xBAD0C0DE, "Failed to detour process: error code = %08x\n", hr);
	}
#endif // DEDICATED
}

void WinSys_Shutdown()
{
#ifdef DEDICATED
	///////////////////////////////////////////////////////////////////////////
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	///////////////////////////////////////////////////////////////////////////
	DetourDetach(&(LPVOID&)VGetVersionExA, (PBYTE)HGetVersionExA);
	DetourDetach(&(LPVOID&)VPeekMessageA, (PBYTE)HPeekMessage);
	//DetourDetach(&(LPVOID&)VPeekMessageW, (PBYTE)HPeekMessage);

	///////////////////////////////////////////////////////////////////////////
	DetourTransactionCommit();
#endif // DEDICATED
}
