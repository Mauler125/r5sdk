#pragma once

BOOL
WINAPI
ConsoleHandlerRoutine(
	DWORD eventCode);

void WinSys_Init();
void WinSys_Shutdown();
