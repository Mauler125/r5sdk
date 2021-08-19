#include "pch.h"
#include "r5dev.h"
#include "id3dx.h"
#include "input.h"
#include "hooks.h"
#include "opcptc.h"
#include "console.h"

//#############################################################################
// INITIALIZATION
//#############################################################################

void InitializeR5Dev()
{
    SetupConsole();
    Hooks::InstallHooks();
    InstallOpcodes();
    g_GuiConfig.Load(); // Load gui config.
    SetupDXSwapChain();
    printf("+-----------------------------------------------------------------------------+\n");
    printf("|   R5 DEV -- INITIALIZED -------------------------------------------------   |\n");
    printf("+-----------------------------------------------------------------------------+\n");
    printf("\n");
}

void TerminateR5Dev()
{
    RemoveDXHooks();
    Hooks::RemoveHooks();
    FreeConsole();
}

//#############################################################################
// ENTRYPOINT
//#############################################################################

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
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
