#include "core/stdafx.h"
#include "core/r5dev.h"
#include "core/init.h"
#include "core/logdef.h"
#include "core/logger.h"
#include "tier0/basetypes.h"
#include "tier0/crashhandler.h"
/*****************************************************************************/
#ifndef DEDICATED
#include "windows/id3dx.h"
#include "windows/input.h"
#endif // !DEDICATED
#include "windows/console.h"
#include "windows/system.h"
#include "mathlib/mathlib.h"
#include "launcher/launcher.h"

#ifndef DEDICATED
#define SDK_DEFAULT_CFG "cfg/startup_default.cfg"
#else
#define SDK_DEFAULT_CFG "cfg/startup_dedi_default.cfg"
#endif

//#############################################################################
// INITIALIZATION
//#############################################################################

void Crash_Callback()
{
    // Shutdown SpdLog to flush all buffers.
    SpdLog_Shutdown();

    // TODO[ AMOS ]: This is where we want to call backtrace from.
}

void Tier0_Init()
{
#if !defined (DEDICATED)
    g_GameDll = CModule("r5apex.exe");
    g_RadVideoToolsDll = CModule("bink2w64.dll");
    g_RadAudioDecoderDll = CModule("binkawin64.dll");
    g_RadAudioSystemDll = CModule("mileswin64.dll");
#if !defined (CLIENT_DLL)
    g_SDKDll = CModule("gamesdk.dll");
#else // This dll is loaded from 'bin/x64_retail//'
    g_SDKDll = CModule("client.dll");
#endif // !CLIENT_DLL
#else // No DirectX and Miles imports.
    g_GameDll = CModule("r5apex_ds.exe");
    g_SDKDll = CModule("dedicated.dll");
#endif // !DEDICATED

    // Setup logger callback sink.
    g_CoreMsgVCallback = &EngineLoggerSink;

    // Setup crash callback.
    g_CrashHandler->SetCrashCallback(&Crash_Callback);
}

void SDK_Init()
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
    if (g_svCmdLine.find("-wconsole") != std::string::npos)
    {
        Console_Init();
    }
#else
    Console_Init();
#endif // !DEDICATED

    lzham_enable_fail_exceptions(true);
    curl_global_init(CURL_GLOBAL_ALL);
    SpdLog_Init();
    Winsock_Init(); // Initialize Winsock.

    for (size_t i = 0; i < SDK_ARRAYSIZE(R5R_EMBLEM); i++)
    {
        std::string svEscaped = StringEscape(R5R_EMBLEM[i]);
        spdlog::info("{:s}{:s}{:s}\n", g_svRedF, svEscaped, g_svReset);
    }
    spdlog::info("\n");

    Systems_Init();
    WinSys_Init();

#ifndef DEDICATED
    Input_Init();
#endif // !DEDICATED
}

//#############################################################################
// SHUTDOWN
//#############################################################################

void SDK_Shutdown()
{
    static bool bShutDown = false;
    assert(!bShutDown);
    if (bShutDown)
    {
        spdlog::error("Recursive shutdown!\n");
        return;
    }
    bShutDown = true;
    spdlog::info("Shutdown GameSDK\n");

    curl_global_cleanup();

    Winsock_Shutdown();
    Systems_Shutdown();
    WinSys_Shutdown();

#ifndef DEDICATED
    Input_Shutdown();
    DirectX_Shutdown();
#endif // !DEDICATED

    Console_Shutdown();
    SpdLog_Shutdown();
}

//#############################################################################
// ENTRYPOINT
//#############################################################################

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    CheckCPU(); // Check CPU as early as possible; error out if CPU isn't supported.
    MathLib_Init(); // Initialize Mathlib.

    NOTE_UNUSED(hModule);
    NOTE_UNUSED(lpReserved);

#if !defined (DEDICATED) && !defined (CLIENT_DLL)
    // This dll is imported by the game executable, we cannot circumvent it.
    // To solve the recursive init problem, we check if -noworkerdll is passed.
    // If this is passed, the worker dll will not be initialized, which allows 
    // us to load the client dll (or any other dll) instead, or load the game
    // without the SDK.
    s_bNoWorkerDll = !!strstr(GetCommandLineA(), "-noworkerdll");
#endif // !DEDICATED && CLIENT_DLL
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            if (!s_bNoWorkerDll)
            {
                Tier0_Init();
                SDK_Init();
            }
            else // Destroy crash handler.
            {
                g_CrashHandler->~CCrashHandler();
                g_CrashHandler = nullptr;
            }
            break;
        }

        case DLL_PROCESS_DETACH:
        {
            if (!s_bNoWorkerDll)
            {
                SDK_Shutdown();
            }
            break;
        }
    }

    return TRUE;
}
