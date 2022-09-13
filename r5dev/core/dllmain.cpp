#include "core/stdafx.h"
#include "core/r5dev.h"
#include "core/init.h"
#include "core/logdef.h"
/*****************************************************************************/
#ifndef DEDICATED
#include "windows/id3dx.h"
#include "windows/input.h"
#endif // !DEDICATED
#include "windows/console.h"
#include "windows/system.h"
#include "launcher/launcher.h"

//#############################################################################
// INITIALIZATION
//#############################################################################

void SDK_Init()
{
    CheckCPU(); // Check CPU as early as possible, SpdLog also uses SIMD intrinsics.

    if (strstr(GetCommandLineA(), "-launcher"))
    {
        g_svCmdLine = GetCommandLineA();
    }
    else
    {
        g_svCmdLine = LoadConfigFile(SDK_DEFAULT_CFG);
    }
#ifndef DEDICATED
    if (g_svCmdLine.find("-wconsole") != std::string::npos)
    {
        Console_Init();
    }
#else
    Console_Init();
#endif // !DEDICATED
    SpdLog_Init();
    spdlog::info("\n");
    for (size_t i = 0; i < SDK_ARRAYSIZE(R5R_EMBLEM); i++)
    {
        std::string svEscaped = StringEscape(R5R_EMBLEM[i]);
        spdlog::info("{:s}{:s}{:s}\n", g_svRedF, svEscaped, g_svReset);
    }
    spdlog::info("\n");

    Systems_Init();
    WinSys_Attach();

#ifndef DEDICATED
    Input_Init();
    DirectX_Init();
#endif // !DEDICATED
}

//#############################################################################
// SHUTDOWN
//#############################################################################

void SDK_Shutdown()
{
    static bool bShutDown = false;
    if (bShutDown)
    {
        spdlog::error("Recursive shutdown!\n");
        return;
    }
    bShutDown = true;
    spdlog::info("Shutdown GameSDK\n");

    Systems_Shutdown();
    WinSys_Detach();

#ifndef DEDICATED
    Input_Shutdown();
    DirectX_Shutdown();
#endif // !DEDICATED

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
            SDK_Init();
            break;
        }

        case DLL_PROCESS_DETACH:
        {
            SDK_Shutdown();
            break;
        }
    }

    return TRUE;
}
