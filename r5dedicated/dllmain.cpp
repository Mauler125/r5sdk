#include "pch.h"
#include "dllmain.h"
#include "hooks.h"
#include "console.h"

//#############################################################################
// INITIALIZATION
//#############################################################################

void InitializeR5Dedicated()
{
    SetupConsole();
    Hooks::InstallHooks();
    Hooks::DedicatedPatch();
    printf("+-----------------------------------------------------------------------------+\n");
    printf("|   R5 DEDICATED SERVER ---------------------------------------------------   |\n");
    printf("+-----------------------------------------------------------------------------+\n");
    printf("\n");
}

void TerminateR5Dedicated()
{
    FreeConsole();
    Hooks::RemoveHooks();
}

//#############################################################################
// ENTRYPOINT
//#############################################################################

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  dwReason, LPVOID lpReserved)
{

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            InitializeR5Dedicated();
            break;
        }

        case DLL_PROCESS_DETACH:
        {
            TerminateR5Dedicated();
            break;
        }
    }

    return TRUE;
}
