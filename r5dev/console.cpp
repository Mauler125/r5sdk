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
	CHAR sBuildBuf[1024] = { 0 };
	fopen_s(&sBuildTxt, "build.txt", "r");
	if (sBuildTxt)
	{
		while (fgets(sBuildBuf, sizeof(sBuildBuf), sBuildTxt) != NULL)
		{
			fclose(sBuildTxt);
		}
	}
	SetConsoleTitle(sBuildBuf);

	// Open input/output streams
	FILE* fDummy;
	freopen_s(&fDummy, "CONIN$", "r", stdin);
	freopen_s(&fDummy, "CONOUT$", "w", stdout);
	freopen_s(&fDummy, "CONOUT$", "w", stderr);

	// Create a worker thread to process console commands
	DWORD threadId;
	DWORD __stdcall ProcessConsoleWorker(LPVOID);
	HANDLE hThread = CreateThread(NULL, 0, ProcessConsoleWorker, NULL, 0, &threadId);

	if (hThread)
	{
		printf("THREAD ID: %ld\n\n", threadId);
		CloseHandle(hThread);
	}
}

static bool b_DebugConsole = true;
bool Hook_ConVar_IsFlagSet(int** cvar, int flag)
{
	int real_flags = *(*(cvar + (72 / (sizeof(void*)))) + (56 / sizeof(int)));
	if (!b_DebugConsole)
	{
		printf("----------------------------------------------\n");
		printf(" Flaged: %08X\n", real_flags);
	}

	// Mask off FCVAR_CHEATS and FCVAR_DEVELOPMENTONLY
	real_flags &= 0xFFFFBFFD;
	if (!b_DebugConsole)
	{
		printf(" Masked: %08X\n", real_flags);
		printf(" Verify: %08X\n", flag);
	}
	if (flag & 0x80000)
	{
		return true;
	}

	return (real_flags & flag) != 0;
}

bool Hook_ConCommand_IsFlagSet(int* cmd, int flag)
{
	int real_flags = *((cmd + (56 / sizeof(int))));
	if (!b_DebugConsole)
	{
		printf("----------------------------------------------\n");
		printf(" Flaged: %08X\n", real_flags);
	}

	// Mask off FCVAR_CHEATS and FCVAR_DEVELOPMENTONLY
	real_flags &= 0xFFFFBFFD;
	if (!b_DebugConsole)
	{
		printf(" Masked: %08X\n", real_flags);
		printf(" Verify: %08X\n", flag);
	}
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
		if (sCommand == "pattern test")
		{
			PrintHAddress();
			continue;
		}
		if (sCommand == "console test")
		{
			b_DebugConsole = !b_DebugConsole;
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