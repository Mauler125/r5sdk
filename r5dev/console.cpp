#include <string>
#include <iostream>
#include <sstream>
#include <Windows.h>

#include <detours.h>

#include "patterns.h"

DWORD __stdcall ProcessConsoleWorker(LPVOID);
FILE* sBuildTxt;
CHAR sBuildBuf[1024];

void SetupConsole()
{
	// Create the console window
	if (AllocConsole() == FALSE)
	{
		OutputDebugString("Failed to create console window!\n");
		return;
	}

	// Set the window title
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
    HANDLE hThread = CreateThread(NULL, NULL, ProcessConsoleWorker, NULL, NULL, NULL);
    CloseHandle(hThread);
}

bool Hook_Cvar_IsFlagSet(int ** cvar, int flag)
{
	int real_flags = *(*(cvar + (72/(sizeof(void*)))) + (56/sizeof(int)));
#ifdef _DEBUG
	printf("----------------------------------------------\n");
	printf("Real flags: %08X\n", real_flags);
#endif
    // mask off FCVAR_CHEATS and FCVAR_DEVELOPMENTONLY
	real_flags &= 0xFFFFBFFD;
#ifdef _DEBUG
	printf("    masked: %08X\n", real_flags);
	printf("  checking: %08X\n", flag);
#endif
	if (flag & 0x80000)
	{
		return true;
	}

	return (real_flags & flag) != 0;
}

bool g_dev = false;
void ToggleDevCommands()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	if (!g_dev)
		DetourAttach((PVOID*)&Cvar_IsFlagSet, &Hook_Cvar_IsFlagSet);
	else
		DetourDetach((PVOID*)&Cvar_IsFlagSet, &Hook_Cvar_IsFlagSet);

	if (DetourTransactionCommit() != NO_ERROR)
		TerminateProcess(GetCurrentProcess(), 0xBAD0C0DE);

	g_dev = ~g_dev;
}

DWORD __stdcall ProcessConsoleWorker(LPVOID)
{

    // Loop forever.
    while (true)
    {
        std::string sCommand;

        // get the user input on the debug console
        printf(">");
        std::getline(std::cin, sCommand);

        if (sCommand == "toggle dev")
        {
            ToggleDevCommands();
            continue;
        }

        // execute the command in the r5 SQVM
        CommandExecute(NULL, sCommand.c_str());
        sCommand.clear();

        // Sleep and loop.
        Sleep(50);
    }

    return 0;
}
