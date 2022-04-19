#include "core/stdafx.h"
#include "windows/system.h"

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
	// Return false for dedicated to reduce unneccesary overhead when calling 'PeekMessageA/W()' every frame.
	return NULL;
#else
	return VPeekMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
#endif // DEDICATED
}

//#############################################################################
// MANAGEMENT
//#############################################################################

void WinSys_Init()
{
	VGetVersionExA = (IGetVersionExA)DetourFindFunction("KERNEL32.dll", "GetVersionExA");
	VPeekMessageA = (IPeekMessage)DetourFindFunction("USER32.dll", "PeekMessageA");
	VPeekMessageW = (IPeekMessage)DetourFindFunction("USER32.dll", "PeekMessageW");
}

void WinSys_Attach()
{
#ifdef DEDICATED
	WinSys_Init();

	///////////////////////////////////////////////////////////////////////////
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	///////////////////////////////////////////////////////////////////////////
	DetourAttach(&(LPVOID&)VGetVersionExA, (PBYTE)HGetVersionExA);
	DetourAttach(&(LPVOID&)VPeekMessageA, (PBYTE)HPeekMessage);
	//DetourAttach(&(LPVOID&)VPeekMessageW, (PBYTE)HPeekMessage);

	///////////////////////////////////////////////////////////////////////////
	DetourTransactionCommit();
#endif // DEDICATED
}

void WinSys_Detach()
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
