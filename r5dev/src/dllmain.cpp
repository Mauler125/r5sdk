#include <Windows.h>
#include <string>

#include "r5dev.h"
#include "hooks.h"
#include "opcptc.h"
#include "console.h"
#include "utility.h"

//---------------------------------------------------------------------------------
// Init
//---------------------------------------------------------------------------------

void InitializeR5Dev()
{
	SetupConsole();
    InstallHooks();
    InstallOpcodes();
    printf("+-----------------------------------------------------------------------------+\n");
    printf("|   R5 DEV -- INITIALIZED -------------------------------------------------   |\n");
    printf("+-----------------------------------------------------------------------------+\n");
    printf("\n");
}

void TerminateR5Dev()
{
    RemoveHooks();
	FreeConsole();
}

//---------------------------------------------------------------------------------
// Main
//---------------------------------------------------------------------------------

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
