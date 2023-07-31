//=============================================================================//
//
// Purpose: Windows window procedure utilities
//
//=============================================================================//
#include "window.h"

//-----------------------------------------------------------------------------
// Purpose: forces the window handle to the front
// Input  : hwnd - 
// Output : non-zero on success, null otherwise
//-----------------------------------------------------------------------------
BOOL ForceForegroundWindow(HWND hwnd)
{
    BOOL ret = TRUE;

    DWORD windowThreadProcessId = GetWindowThreadProcessId(GetForegroundWindow(), LPDWORD(0));
    DWORD currentThreadId = GetCurrentThreadId();

    if (!AttachThreadInput(windowThreadProcessId, currentThreadId, true))
        ret = FALSE;
    if (!BringWindowToTop(hwnd))
        ret = FALSE;
    if (!ShowWindow(hwnd, SW_SHOW))
        ret = FALSE;
    if (!AttachThreadInput(windowThreadProcessId, currentThreadId, false))
        ret = FALSE;

    return ret;
}
