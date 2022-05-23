#include "core/stdafx.h"
#include "sdklauncher.h"
#include "basepanel.h"
#include <objidl.h>
#include "gdiplus.h"
#include "shellapi.h"

using namespace Gdiplus;

#pragma comment (lib,"Shell32.lib")
#pragma comment (lib,"Gdi32.lib")
#pragma comment (lib,"Gdiplus.lib")
#pragma comment (lib,"Advapi32.lib")

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
    case eLaunchMode::LM_DEBUG_GAME:
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
    case eLaunchMode::LM_RELEASE_GAME:
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
    case eLaunchMode::LM_DEBUG_DEDI:
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
    case eLaunchMode::LM_RELEASE_DEDI:
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

        CUIBasePanel* mainUI = new CUIBasePanel();
        Forms::Application::Run(mainUI);
        UIX::UIXTheme::ShutdownRenderer();
    }
    else
    {
        for (int i = 1; i < argc; ++i)
        {
            std::string arg = argv[i];
            if ((arg == "-debug") || (arg == "-dbg"))
            {
                if (g_pLauncher->Setup(eLaunchMode::LM_DEBUG_GAME, eLaunchState::LS_CHEATS))
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
                if (g_pLauncher->Setup(eLaunchMode::LM_RELEASE_GAME, eLaunchState::LS_CHEATS))
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
                if (g_pLauncher->Setup(eLaunchMode::LM_DEBUG_DEDI, eLaunchState::LS_CHEATS))
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
                if (g_pLauncher->Setup(eLaunchMode::LM_RELEASE_DEDI, eLaunchState::LS_CHEATS))
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
        spdlog::warn("Use DEBUG GAME        [1] for research and development purposes.\n");
        spdlog::warn("Use RELEASE GAME      [2] for playing the game and creating servers.\n");
        spdlog::warn("Use DEBUG DEDICATED   [3] for research and development purposes.\n");
        spdlog::warn("Use RELEASE DEDICATED [4] for running and hosting dedicated servers.\n");
        std::cout << "----------------------------------------------------------------------------------------------------------------------" << std::endl;
        spdlog::info("Enter '1' for 'DEBUG GAME'.\n");
        spdlog::info("Enter '2' for 'RELEASE GAME'.\n");
        spdlog::info("Enter '3' for 'DEBUG DEDICATED'.\n");
        spdlog::info("Enter '4' for 'RELEASE DEDICATED'.\n");
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
                case eLaunchMode::LM_DEBUG_GAME:
                {
                    if (g_pLauncher->Setup(eLaunchMode::LM_DEBUG_GAME, eLaunchState::LS_CHEATS))
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
                case eLaunchMode::LM_RELEASE_GAME:
                {
                    if (g_pLauncher->Setup(eLaunchMode::LM_RELEASE_GAME, eLaunchState::LS_CHEATS))
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
                case eLaunchMode::LM_DEBUG_DEDI:
                {
                    if (g_pLauncher->Setup(eLaunchMode::LM_DEBUG_DEDI, eLaunchState::LS_CHEATS))
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
                case eLaunchMode::LM_RELEASE_DEDI:
                {
                    if (g_pLauncher->Setup(eLaunchMode::LM_RELEASE_DEDI, eLaunchState::LS_CHEATS))
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
