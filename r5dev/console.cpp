#include <string>
#include <sstream>
#include <iostream>

#include <Windows.h>
#include <detours.h>

#include "hooks.h"
#include "opcptc.h"
#include "console.h"
#include "patterns.h"

//---------------------------------------------------------------------------------
// Init
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
	DWORD threadId0;
	DWORD __stdcall ProcessConsoleWorker(LPVOID);
	HANDLE hThread0 = CreateThread(NULL, 0, ProcessConsoleWorker, NULL, 0, &threadId0);

	if (hThread0)
	{
		printf("THREAD ID: %ld\n\n", threadId0);
		CloseHandle(hThread0);
	}
}

//---------------------------------------------------------------------------------
// Hooks
//---------------------------------------------------------------------------------

bool Hook_ConVar_IsFlagSet(int** cvar, int flag)
{
	int real_flags = *(*(cvar + (72 / (sizeof(void*)))) + (56 / sizeof(int)));
	if (g_bDebugConsole)
	{
		printf("--------------------------------------------------\n");
		printf(" Flaged: %08X\n", real_flags);
	}
	// Mask off FCVAR_CHEATS and FCVAR_DEVELOPMENTONLY
	real_flags &= 0xFFFFBFFD;
	if (g_bDebugConsole)
	{
		printf(" Masked: %08X\n", real_flags);
		printf(" Verify: %08X\n", flag);
		printf("--------------------------------------------------\n");
	}
	if (flag & 0x80000) { return true; }

	if (!g_bReturnAllFalse) { return (real_flags & flag) != 0; }
	else { return false; }
}

bool Hook_ConCommand_IsFlagSet(int* cmd, int flag)
{
	int real_flags = *((cmd + (56 / sizeof(int))));
	if (g_bDebugConsole)
	{
		printf("--------------------------------------------------\n");
		printf(" Flaged: %08X\n", real_flags);
	}
	// Mask off FCVAR_CHEATS and FCVAR_DEVELOPMENTONLY
	real_flags &= 0xFFFFBFFD;
	if (g_bDebugConsole)
	{
		printf(" Masked: %08X\n", real_flags);
		printf(" Verify: %08X\n", flag);
		printf("--------------------------------------------------\n");
	}
	if (flag & 0x80000) { return true; }

	if (!g_bReturnAllFalse) { return (real_flags & flag) != 0; }
	else { return false; }
}

//---------------------------------------------------------------------------------
// Worker
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

		// Engine toggles
		if (sCommand == "toggle net") { ToggleNetHooks(); continue; }
		if (sCommand == "toggle dev") { ToggleDevCommands(); continue; }
		if (sCommand == "toggle fal") { g_bReturnAllFalse = !g_bReturnAllFalse; continue; }

		// Debug toggles
		if (sCommand == "pattern test") { PrintHAddress(); PrintOAddress(); continue; }
		if (sCommand == "console test") { g_bDebugConsole = !g_bDebugConsole; continue; }

		// Exec toggles
		if (sCommand == "1") { ToggleDevCommands(); CommandExecute(NULL, "exec autoexec_dev"); }
		if (sCommand == "2") { g_bDebugLog = !g_bDebugLog; continue; }

		// Execute the command in the r5 SQVM
		CommandExecute(NULL, sCommand.c_str());
		sCommand.clear();

		// Sleep and loop
		Sleep(50);
	}

	return 0;
}
