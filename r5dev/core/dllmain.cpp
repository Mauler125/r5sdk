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

void R5Dev_Init()
{
    if (strstr(GetCommandLineA(), "-launcher"))
    {
        g_svCmdLine = GetCommandLineA();
    }
    else
    {
        g_svCmdLine = LoadConfigFile(SDK_DEFAULT_CFG);
    }
#ifndef DEDICATED
    if (strstr(g_svCmdLine.c_str(), "-wconsole"))
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
        spdlog::info("{:s}{:s}{:s}\n", g_svRedF.c_str(), svEscaped.c_str(), g_svReset.c_str());
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

void R5Dev_Shutdown()
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
            R5Dev_Init();
            break;
        }

        case DLL_PROCESS_DETACH:
        {
            R5Dev_Shutdown();
            break;
        }
    }

    return TRUE;
}
