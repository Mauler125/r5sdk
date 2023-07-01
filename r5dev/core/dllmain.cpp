#include "core/stdafx.h"
#include "core/r5dev.h"
#include "core/init.h"
#include "core/logdef.h"
#include "core/logger.h"
#include "tier0/basetypes.h"
#include "tier0/crashhandler.h"
#include "tier0/commandline.h"
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

bool g_bSdkInitialized = false;

//#############################################################################
// UTILITY
//#############################################################################

void Crash_Callback()
{
    // Shutdown SpdLog to flush all buffers.
    SpdLog_Shutdown();

    // TODO[ AMOS ]: This is where we want to call backtrace from.
}

void Show_Emblem()
{
    // Logged as 'SYSTEM_ERROR' for its red color.
    for (size_t i = 0; i < SDK_ARRAYSIZE(R5R_EMBLEM); i++)
    {
        DevMsg(eDLL_T::SYSTEM_ERROR, "%s\n", R5R_EMBLEM[i]);
    }

    // Log the SDK's 'build_id' under the emblem.
    DevMsg(eDLL_T::SYSTEM_ERROR, "+------------------------------------------------[%010d]-+\n",
        g_SDKDll.GetNTHeaders()->FileHeader.TimeDateStamp);
    DevMsg(eDLL_T::SYSTEM_ERROR, "\n");
}

//#############################################################################
// INITIALIZATION
//#############################################################################

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

    g_pCmdLine = g_GameDll.GetExportedSymbol("g_pCmdLine").RCast<CCommandLine*>();
    g_CoreMsgVCallback = &EngineLoggerSink; // Setup logger callback sink.

    g_pCmdLine->CreateCmdLine(GetCommandLineA());
    g_CrashHandler->SetCrashCallback(&Crash_Callback);

    // This prevents the game from recreating it,
    // see 'CCommandLine::StaticCreateCmdLine' for
    // more information.
    g_bCommandLineCreated = true;
}

void SDK_Init()
{
    Tier0_Init();

    if (!CommandLine()->CheckParm("-launcher"))
    {
        CommandLine()->AppendParametersFromFile(SDK_DEFAULT_CFG);
    }

    const bool bAnsiColor = CommandLine()->CheckParm("-ansicolor") ? true : false;

#ifndef DEDICATED
    if (CommandLine()->CheckParm("-wconsole"))
#endif // !DEDICATED
    {
        Console_Init(bAnsiColor);
    }

    SpdLog_Init(bAnsiColor);
    Show_Emblem();

    Winsock_Init(); // Initialize Winsock.
    Systems_Init();

    WinSys_Init();
#ifndef DEDICATED
    Input_Init();
#endif // !DEDICATED

    curl_global_init(CURL_GLOBAL_ALL);
    lzham_enable_fail_exceptions(true);

    g_bSdkInitialized = true;
}

//#############################################################################
// SHUTDOWN
//#############################################################################

void SDK_Shutdown()
{
    assert(g_bSdkInitialized);

    if (!g_bSdkInitialized)
    {
        spdlog::error("Recursive shutdown!\n");
        return;
    }

    g_bSdkInitialized = false;
    spdlog::info("Shutdown GameSDK\n");

    curl_global_cleanup();

#ifndef DEDICATED
    Input_Shutdown();
#endif // !DEDICATED

    WinSys_Shutdown();
    Systems_Shutdown();
    Winsock_Shutdown();

    SpdLog_Shutdown();
    Console_Shutdown();
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
