#include <Windows.h>
#include <string>

#include "r5dev.h"
#include "id3dx.h"
#include "input.h"
#include "hooks.h"
#include "opcptc.h"
#include "console.h"
#include "utility.h"
#include "gameclasses.h"

//#############################################################################
// INITIALIZATION
//#############################################################################

void InitializeR5Dev()
{
    SetupConsole();
    InstallENHooks();
    InstallIPHooks();
    InstallDXHooks();
    InstallOpcodes();
    SetupDXSwapChain();
    printf("+-----------------------------------------------------------------------------+\n");
    printf("|   R5 DEV -- INITIALIZED -------------------------------------------------   |\n");
    printf("+-----------------------------------------------------------------------------+\n");
    printf("\n");
}

void TerminateR5Dev()
{
    RemoveCMHooks();
    RemoveENHooks();
    RemoveIPHooks();
    RemoveDXHooks();
    FreeConsole();
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
