#include <string>
#include <iostream>
#include <sstream>
#include <Windows.h>

#include <detours.h>

// Build 313 R5PC_J1624A_CL394493_2019_02_24_09_29_PM
void (*CommandExecute)(void * self, const char* cmd) = (void (*)(void*, const char*))0x140244900; /*48 89 5C 24 ? 57 48 83 EC 20 48 8D 0D ? ? ? ? 41 8B D8*/

DWORD __stdcall ProcessConsoleWorker(LPVOID);

void SetupConsole()
{
	// Create the console window
	if (AllocConsole() == FALSE)
	{
		OutputDebugString("Failed to create console window!\n");
		return;
	}

	// Set the window title
	SetConsoleTitle("R5PC_J1624A_CL394493_2019_02_24_09_29_PM");

	// Open input/output streams
	FILE* fDummy;
	freopen_s(&fDummy, "CONIN$", "r", stdin);
	freopen_s(&fDummy, "CONOUT$", "w", stdout);
	freopen_s(&fDummy, "CONOUT$", "w", stderr);

    // Create a worker thread to process console commands.
    HANDLE hThread = CreateThread(NULL, NULL, ProcessConsoleWorker, NULL, NULL, NULL);
    CloseHandle(hThread);
}

bool (*Cvar_IsFlagSet)(int ** cvar, int flag) = (bool (*)(int**, int))0x1404C87C0;
bool Hook_Cvar_IsFlagSet(int ** cvar, int flag); /*48 8B 41 48 85 50 38*/

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
