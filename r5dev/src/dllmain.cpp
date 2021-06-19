#include <Windows.h>
#include <string>

#include "r5dev.h"
#include "id3dx.h"
#include "hooks.h"
#include "opcptc.h"
#include "console.h"
#include "utility.h"

///////////////////////////////////////////////////////////////////////////////////
//---------------------------------------------------------------------------------
// Init
//---------------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////////////
//---------------------------------------------------------------------------------
// Entry
//---------------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  dwReason, LPVOID lpReserved)
{

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            InitializeR5Dev();
            SetupDXSwapChain();
            break;
        }

        case DLL_PROCESS_DETACH:
        {
            TerminateR5Dev();
            RemoveDXHooks();
            break;
        }
    }

    return TRUE;
}
