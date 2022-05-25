#include "core/stdafx.h"
#include "basepanel.h"
#include "sdklauncher_const.h"
#include "sdklauncher.h"
#include <objidl.h>
#include "gdiplus.h"
#include "shellapi.h"

using namespace Gdiplus;

#pragma comment (lib,"Shell32.lib")
#pragma comment (lib,"Gdi32.lib")
#pragma comment (lib,"Gdiplus.lib")
#pragma comment (lib,"Advapi32.lib")

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

//-----------------------------------------------------------------------------
// Purpose switch case:
// * Launch the game in user specified mode and state.
// * Load specified command line arguments from a file on the disk.
// * Format the file paths for the game exe and specified hook dll.
//-----------------------------------------------------------------------------
bool CLauncher::Setup(eLaunchMode lMode, eLaunchState lState)
{
    ///////////////////////////////////////////////////////////////////////////
    std::string svCmdLineArgs              = std::string();

    ///////////////////////////////////////////////////////////////////////////
    switch (lMode)
    {
    case eLaunchMode::LM_HOST_DEBUG:
    {
        fs::path cfgPath = fs::current_path() /= "platform\\cfg\\startup_debug.cfg";
        std::ifstream cfgFile(cfgPath);
        if (cfgFile.good() && cfgFile)
        {
            std::stringstream ss;
            ss << cfgFile.rdbuf();
            svCmdLineArgs = ss.str() + "-launcher";
        }
        else
        {
            spdlog::error("File 'platform\\cfg\\startup_debug.cfg' does not exist!\n");
            cfgFile.close();
            return false;
        }
        cfgFile.close(); // Close cfg file.

        m_svWorkerDll = m_svCurrentDir + "\\gamesdk.dll";
        m_svGameExe   = m_svCurrentDir + "\\r5apex.exe";
        m_svCmdLine   = m_svCurrentDir + "\\r5apex.exe " + svCmdLineArgs;

        spdlog::info("*** LAUNCHING GAME [DEBUG] ***\n");
        break;
    }
    case eLaunchMode::LM_HOST:
    {
        fs::path cfgPath = fs::current_path() /= "platform\\cfg\\startup_retail.cfg";
        std::ifstream cfgFile(cfgPath);
        if (cfgFile.good() && cfgFile)
        {
            std::stringstream ss;
            ss << cfgFile.rdbuf();
            svCmdLineArgs = ss.str() + "-launcher";
        }
        else
        {
            spdlog::error("File 'platform\\cfg\\startup_retail.cfg' does not exist!\n");
            cfgFile.close();
            return false;
        }
        cfgFile.close(); // Close cfg file.

        m_svWorkerDll = m_svCurrentDir + "\\gamesdk.dll";
        m_svGameExe   = m_svCurrentDir + "\\r5apex.exe";
        m_svCmdLine   = m_svCurrentDir + "\\r5apex.exe " + svCmdLineArgs;

        spdlog::info("*** LAUNCHING GAME [RELEASE] ***\n");
        break;
    }
    case eLaunchMode::LM_SERVER_DEBUG:
    {
        fs::path cfgPath = fs::current_path() /= "platform\\cfg\\startup_dedi_debug.cfg";
        std::ifstream cfgFile(cfgPath);
        if (cfgFile.good() && cfgFile)
        {
            std::stringstream ss;
            ss << cfgFile.rdbuf();
            svCmdLineArgs = ss.str() + "-launcher";
        }
        else
        {
            spdlog::error("File 'platform\\cfg\\startup_dedi_debug.cfg' does not exist!\n");
            cfgFile.close();
            return false;
        }
        cfgFile.close(); // Close cfg file.

        m_svWorkerDll = m_svCurrentDir + "\\dedicated.dll";
        m_svGameExe   = m_svCurrentDir + "\\r5apex_ds.exe";
        m_svCmdLine   = m_svCurrentDir + "\\r5apex_ds.exe " + svCmdLineArgs;

        spdlog::info("*** LAUNCHING DEDICATED [DEBUG] ***\n");
        break;
    }
    case eLaunchMode::LM_SERVER:
    {
        fs::path cfgPath = fs::current_path() /= "platform\\cfg\\startup_dedi_retail.cfg";
        std::ifstream cfgFile(cfgPath);
        if (cfgFile.good() && cfgFile)
        {
            std::stringstream ss;
            ss << cfgFile.rdbuf();
            svCmdLineArgs = ss.str(); +"-launcher";
        }
        else
        {
            spdlog::error("File 'platform\\cfg\\startup_dedi_retail.cfg' does not exist!\n");
            cfgFile.close();
            return false;
        }
        cfgFile.close(); // Close cfg file.

        m_svWorkerDll = m_svCurrentDir + "\\dedicated.dll";
        m_svGameExe   = m_svCurrentDir + "\\r5apex_ds.exe";
        m_svCmdLine   = m_svCurrentDir + "\\r5apex_ds.exe " + svCmdLineArgs;

        spdlog::info("*** LAUNCHING DEDICATED [RELEASE] ***\n");
        break;
    }
    default:
    {
        spdlog::error("*** NO LAUNCH MODE SPECIFIED ***\n");
        return false;
    }
    }

    ///////////////////////////////////////////////////////////////////////////
// Print the file paths and arguments.
    std::cout << "----------------------------------------------------------------------------------------------------------------------" << std::endl;
    spdlog::debug("- CWD: {}\n", m_svCurrentDir);
    spdlog::debug("- EXE: {}\n", m_svGameExe);
    spdlog::debug("- DLL: {}\n", m_svWorkerDll);
    spdlog::debug("- CLI: {}\n", svCmdLineArgs);
    std::cout << "----------------------------------------------------------------------------------------------------------------------" << std::endl;

    return true;
}

bool CLauncher::Setup(eLaunchMode lMode, const string& svCommandLine)
{
    ///////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    switch (lMode)
    {
    case eLaunchMode::LM_HOST_DEBUG:
    {
        m_svWorkerDll = m_svCurrentDir + "\\gamesdk.dll";
        m_svGameExe = m_svCurrentDir + "\\r5apex.exe";
        m_svCmdLine = m_svCurrentDir + "\\r5apex.exe " + svCommandLine;

        spdlog::info("*** LAUNCHER SETUP FOR HOST [DEBUG] ***\n");
        break;
    }
    case eLaunchMode::LM_HOST:
    {
        m_svWorkerDll = m_svCurrentDir + "\\gamesdk.dll";
        m_svGameExe = m_svCurrentDir + "\\r5apex.exe";
        m_svCmdLine = m_svCurrentDir + "\\r5apex.exe " + svCommandLine;

        spdlog::info("*** LAUNCHER SETUP FOR HOST [RELEASE] ***\n");
        break;
    }
    case eLaunchMode::LM_SERVER_DEBUG:
    {
        m_svWorkerDll = m_svCurrentDir + "\\dedicated.dll";
        m_svGameExe = m_svCurrentDir + "\\r5apex_ds.exe";
        m_svCmdLine = m_svCurrentDir + "\\r5apex_ds.exe " + svCommandLine;

        spdlog::info("*** LAUNCHER SETUP FOR DEDICATED [DEBUG] ***\n");
        break;
    }
    case eLaunchMode::LM_SERVER:
    {
        m_svWorkerDll = m_svCurrentDir + "\\dedicated.dll";
        m_svGameExe = m_svCurrentDir + "\\r5apex_ds.exe";
        m_svCmdLine = m_svCurrentDir + "\\r5apex_ds.exe " + svCommandLine;

        spdlog::info("*** LAUNCHER SETUP FOR DEDICATED [RELEASE] ***\n");
        break;
    }
    default:
    {
        spdlog::error("*** INVALID LAUNCH MODE SPECIFIED ***\n");
        return false;
    }
    }

    ///////////////////////////////////////////////////////////////////////////
    // Print the file paths and arguments.
    std::cout << "----------------------------------------------------------------------------------------------------------------------" << std::endl;
    spdlog::debug("- CWD: {}\n", m_svCurrentDir);
    spdlog::debug("- EXE: {}\n", m_svGameExe);
    spdlog::debug("- DLL: {}\n", m_svWorkerDll);
    spdlog::debug("- CLI: {}\n", svCommandLine);
    std::cout << "----------------------------------------------------------------------------------------------------------------------" << std::endl;

    return true;
}

bool CLauncher::Launch()
{
    ///////////////////////////////////////////////////////////////////////////
    // Build our list of dlls to inject.
    LPCSTR DllsToInject[1] =
    {
        m_svWorkerDll.c_str()
    };

    STARTUPINFOA StartupInfo = { 0 };
    PROCESS_INFORMATION ProcInfo = { 0 };

    // Initialize startup info struct.
    StartupInfo.cb = sizeof(STARTUPINFOA);

    ///////////////////////////////////////////////////////////////////////////
    // Create the game process in a suspended state with our dll.
    BOOL result = DetourCreateProcessWithDllsA
    (
        m_svGameExe.c_str(),                           // lpApplicationName
        (LPSTR)m_svCmdLine.c_str(),                    // lpCommandLine
        NULL,                                          // lpProcessAttributes
        NULL,                                          // lpThreadAttributes
        FALSE,                                         // bInheritHandles
        CREATE_SUSPENDED,                              // dwCreationFlags
        NULL,                                          // lpEnvironment
        m_svCurrentDir.c_str(),                        // lpCurrentDirectory
        &StartupInfo,                                  // lpStartupInfo
        &ProcInfo,                                     // lpProcessInformation
        sizeof(DllsToInject) / sizeof(LPCSTR),         // nDlls
        DllsToInject,                                  // rlpDlls
        NULL                                           // pfCreateProcessA
    );

    ///////////////////////////////////////////////////////////////////////////
    // Failed to create the process.
    if (!result)
    {
        PrintLastError();
        return false;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Resume the process.
    ResumeThread(ProcInfo.hThread);

    ///////////////////////////////////////////////////////////////////////////
    // Close the process and thread handles.
    CloseHandle(ProcInfo.hProcess);
    CloseHandle(ProcInfo.hThread);

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// Entrypoint.
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[], char* envp[])
{
    spdlog::set_pattern("[%^%l%$] %v");
    spdlog::set_level(spdlog::level::trace);

    if (argc < 2)
    {
        Forms::Application::EnableVisualStyles();
        UIX::UIXTheme::InitializeRenderer(new Themes::KoreTheme());

        g_pLauncher->m_pSurface = new CUIBaseSurface();
        Forms::Application::Run(g_pLauncher->m_pSurface);
        UIX::UIXTheme::ShutdownRenderer();
    }
    else
    {
        for (int i = 1; i < argc; ++i)
        {
            std::string arg = argv[i];
            if ((arg == "-debug") || (arg == "-dbg"))
            {
                if (g_pLauncher->Setup(eLaunchMode::LM_HOST_DEBUG, eLaunchState::LS_CHEATS))
                {
                    if (g_pLauncher->Launch())
                    {
                        Sleep(2000);
                        return EXIT_SUCCESS;
                    }
                }

                Sleep(2000);
                return EXIT_FAILURE;
            }
            if ((arg == "-release") || (arg == "-rel"))
            {
                if (g_pLauncher->Setup(eLaunchMode::LM_HOST, eLaunchState::LS_CHEATS))
                {
                    if (g_pLauncher->Launch())
                    {
                        Sleep(2000);
                        return EXIT_SUCCESS;
                    }
                }

                Sleep(2000);
                return EXIT_FAILURE;
            }
            if ((arg == "-dedicated_dev") || (arg == "-dedid"))
            {
                if (g_pLauncher->Setup(eLaunchMode::LM_SERVER_DEBUG, eLaunchState::LS_CHEATS))
                {
                    if (g_pLauncher->Launch())
                    {
                        Sleep(2000);
                        return EXIT_SUCCESS;
                    }
                }

                Sleep(2000);
                return EXIT_FAILURE;
            }
            if ((arg == "-dedicated") || (arg == "-dedi"))
            {
                if (g_pLauncher->Setup(eLaunchMode::LM_SERVER, eLaunchState::LS_CHEATS))
                {
                    if (g_pLauncher->Launch())
                    {
                        Sleep(2000);
                        return EXIT_SUCCESS;
                    }
                }

                Sleep(2000);
                return EXIT_FAILURE;
            }
        }

        std::cout << "----------------------------------------------------------------------------------------------------------------------" << std::endl;
        spdlog::warn("If a DEBUG option has been choosen as launch parameter, do not broadcast servers to the Server Browser!\n");
        spdlog::warn("All FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY ConVar's/ConCommand's will be enabled.\n");
        spdlog::warn("Connected clients will be able to set and execute anything flagged FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY.\n");
        std::cout << "----------------------------------------------------------------------------------------------------------------------" << std::endl;
        spdlog::warn("Use DEBUG HOST        [1] for research and development purposes.\n");
        spdlog::warn("Use RELEASE HOST      [2] for playing the game and creating servers.\n");
        spdlog::warn("Use DEBUG SERVER      [3] for research and development purposes.\n");
        spdlog::warn("Use RELEASE SERVER    [4] for running and hosting dedicated servers.\n");
        spdlog::warn("Use DEBUG CLIENT      [5] for research and development purposes.\n");
        spdlog::warn("Use RELEASE CLIENT    [6] for running client only builds against remote servers.\n");
        std::cout << "----------------------------------------------------------------------------------------------------------------------" << std::endl;
        spdlog::info("Enter '1' for 'DEBUG HOST'.\n");
        spdlog::info("Enter '2' for 'RELEASE HOST'.\n");
        spdlog::info("Enter '3' for 'DEBUG SERVER'.\n");
        spdlog::info("Enter '4' for 'RELEASE SERVER'.\n");
        spdlog::info("Enter '5' for 'DEBUG CLIENT'.\n");
        spdlog::info("Enter '6' for 'RELEASE CLIENT'.\n");
        std::cout << "----------------------------------------------------------------------------------------------------------------------" << std::endl;
        std::cout << "User input: ";

        std::string input = std::string();
        if (std::cin >> input)
        {
            try
            {
                eLaunchMode mode = (eLaunchMode)std::stoi(input);
                switch (mode)
                {
                case eLaunchMode::LM_HOST_DEBUG:
                {
                    if (g_pLauncher->Setup(eLaunchMode::LM_HOST_DEBUG, eLaunchState::LS_CHEATS))
                    {
                        if (g_pLauncher->Launch())
                        {
                            Sleep(2000);
                            return EXIT_SUCCESS;
                        }
                    }

                    Sleep(2000);
                    return EXIT_FAILURE;
                }
                case eLaunchMode::LM_HOST:
                {
                    if (g_pLauncher->Setup(eLaunchMode::LM_HOST, eLaunchState::LS_CHEATS))
                    {
                        if (g_pLauncher->Launch())
                        {
                            Sleep(2000);
                            return EXIT_SUCCESS;
                        }
                    }

                    Sleep(2000);
                    return EXIT_FAILURE;
                }
                case eLaunchMode::LM_SERVER_DEBUG:
                {
                    if (g_pLauncher->Setup(eLaunchMode::LM_SERVER_DEBUG, eLaunchState::LS_CHEATS))
                    {
                        if (g_pLauncher->Launch())
                        {
                            Sleep(2000);
                            return EXIT_SUCCESS;
                        }
                    }

                    Sleep(2000);
                    return EXIT_FAILURE;
                }
                case eLaunchMode::LM_SERVER:
                {
                    if (g_pLauncher->Setup(eLaunchMode::LM_SERVER, eLaunchState::LS_CHEATS))
                    {
                        if (g_pLauncher->Launch())
                        {
                            Sleep(2000);
                            return EXIT_SUCCESS;
                        }
                    }

                    Sleep(2000);
                    return EXIT_FAILURE;
                }
                default:
                {
                    spdlog::error("R5Reloaded requires '1' for DEBUG GAME mode, '2' for RELEASE GAME mode, '3' for DEBUG DEDICATED mode, '4' for RELEASE DEDICATED mode.\n");
                    Sleep(5000);
                    return EXIT_FAILURE;
                }
                }
            }
            catch (std::exception& e)
            {
                spdlog::error("R5Reloaded only takes numerical input to launch. Error: {}.\n", e.what());
                Sleep(5000);
                return EXIT_FAILURE;
            }
        }
        spdlog::error("R5Reloaded requires numerical input to launch.\n");

        Sleep(5000);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
