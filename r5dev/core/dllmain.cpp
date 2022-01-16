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

//#############################################################################
// INITIALIZATION
//#############################################################################

void R5Dev_Init()
{
#ifndef DEDICATED
    if (strstr(GetCommandLineA(), "-wconsole")) { Console_Init(); }
#else
    Console_Init();
#endif // !DEDICATED

    SpdLog_Init();
    Systems_Init();
    WinSys_Attach();

#ifndef DEDICATED
    Input_Init();
    DirectX_Init();
#endif // !DEDICATED

    spdlog::info("\n");
    for (int i = 0; i < (&R5R_LOGO)[1] - R5R_LOGO; i++)
    {
        std::string unescaped = StringEscape(R5R_LOGO[i]);
        spdlog::info("{}{}{}\n", g_svRedF.c_str(), unescaped.c_str(), g_svReset.c_str());
    }
    spdlog::info("\n");
}

//#############################################################################
// SHUTDOWN
//#############################################################################

void R5Dev_Shutdown()
{
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
