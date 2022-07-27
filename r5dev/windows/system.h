#pragma once

BOOL
WINAPI
ConsoleHandlerRoutine(
	DWORD eventCode);

void WinSys_Attach();
void WinSys_Detach();
