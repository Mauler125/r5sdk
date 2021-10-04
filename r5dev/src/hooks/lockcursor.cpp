#include "pch.h"
#include "hooks.h"
#include "id3dx.h"

namespace Hooks
{
	LockCursorFn originalLockCursor = nullptr;
}

void Hooks::LockCursor(void* thisptr)
{
	if (g_bShowConsole || g_bShowBrowser)
	{
		addr_CMatSystemSurface_UnlockCursor(thisptr); // Unlock cursor if our gui is shown.
		return;
	}
	return originalLockCursor(thisptr);
}