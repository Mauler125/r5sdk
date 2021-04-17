#include <string>
#include <iostream>
#include <sstream>
#include <Windows.h>

#include <detours.h>

#include "hooks.h"
#include "patterns.h"

//---------------------------------------------------------------------------------
// Console Hooks
//---------------------------------------------------------------------------------

void SetupConsole()
{
	// Create the console window
	if (AllocConsole() == FALSE)
	{
		OutputDebugString("Failed to create console window!\n");
		return;
	}

	// Set the window title
	FILE* sBuildTxt;
	CHAR sBuildBuf[1024];
#pragma warning(suppress : 4996)
	sBuildTxt = fopen("build.txt", "r");
	if (sBuildTxt)
	{
		while (fgets(sBuildBuf, sizeof(sBuildBuf), sBuildTxt) != NULL)
			fclose(sBuildTxt);
	}
	SetConsoleTitle(sBuildBuf);

	// Open input/output streams
	FILE* fDummy;
	freopen_s(&fDummy, "CONIN$", "r", stdin);
	freopen_s(&fDummy, "CONOUT$", "w", stdout);
	freopen_s(&fDummy, "CONOUT$", "w", stderr);

    // Create a worker thread to process console commands.
	DWORD __stdcall ProcessConsoleWorker(LPVOID);
    HANDLE hThread = CreateThread(NULL, NULL, ProcessConsoleWorker, NULL, NULL, NULL);
    CloseHandle(hThread);
}

bool Hook_Cvar_IsFlagSet(int ** cvar, int flag)
{
	int real_flags = *(*(cvar + (72/(sizeof(void*)))) + (56/sizeof(int)));

	printf("----------------------------------------------\n");
	printf("Real flags: %08X\n", real_flags);

    // Mask off FCVAR_CHEATS and FCVAR_DEVELOPMENTONLY
	real_flags &= 0xFFFFBFFD;

	printf("    Masked: %08X\n", real_flags);
	printf("  Checking: %08X\n", flag);

	if (flag & 0x80000)
	{
		return true;
	}

	return (real_flags & flag) != 0;
}

//---------------------------------------------------------------------------------
// Console Worker
//---------------------------------------------------------------------------------

DWORD __stdcall ProcessConsoleWorker(LPVOID)
{
	// Loop forever
	while (true)
	{
		std::string sCommand;

		// Get the user input on the debug console
		printf(">");
		std::getline(std::cin, sCommand);

		if (sCommand == "toggle dev")
		{
			ToggleDevCommands();
			continue;
		}

		if (sCommand == "toggle net")
		{
			ToggleNetHooks();
			continue;
		}

		// Execute the command in the r5 SQVM
		CommandExecute(NULL, sCommand.c_str());
		sCommand.clear();

		// Sleep and loop
		Sleep(50);
	}

	return 0;
}