#include <string>
#include <Windows.h>

#include "r5dev.h"
#include "console.h"
#include "utilities.h"
#include "hooks.h"

__declspec(dllexport) void DummyExport()
{
    // Required for detours.
}

//---------------------------------------------------------------------------------
// Main
//---------------------------------------------------------------------------------

void InitializeR5Dev()
{
	SetupConsole();
    InstallHooks();
    printf("R5 Dev -- Initialized...\n");
}

void TerminateR5Dev()
{
    RemoveHooks();
	FreeConsole();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  dwReason, LPVOID lpReserved)
{

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            InitializeR5Dev();
            break;
        }

        case DLL_PROCESS_DETACH:
        {
            TerminateR5Dev();
            break;
        }
    }

    return TRUE;
}