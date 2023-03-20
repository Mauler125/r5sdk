//=============================================================================//
//
// Purpose: SDK launcher implementation.
//
//=============================================================================//
#include "core/stdafx.h"
#include "basepanel.h"
#include "sdklauncher_const.h"
#include "sdklauncher.h"
#include "public/utility/binstream.h"

///////////////////////////////////////////////////////////////////////////////
// Purpose: initializes and runs the user interface
///////////////////////////////////////////////////////////////////////////////
void CLauncher::RunSurface()
{
    Forms::Application::EnableVisualStyles();
    UIX::UIXTheme::InitializeRenderer(new Themes::KoreTheme());

    m_pSurface = new CSurface();
    Forms::Application::Run(g_pLauncher->m_pSurface);
    UIX::UIXTheme::ShutdownRenderer();
}

///////////////////////////////////////////////////////////////////////////////
// Purpose: initializes the console (development only)
///////////////////////////////////////////////////////////////////////////////
void CLauncher::InitConsole()
{
    AllocConsole();
    freopen("conin$", "r", stdin);
    freopen("conout$", "w", stdout);
    freopen("conout$", "w", stderr);
}

///////////////////////////////////////////////////////////////////////////////
// Purpose: initializes the logger
///////////////////////////////////////////////////////////////////////////////
void CLauncher::InitLogger()
{
    m_pLogger->set_pattern("[%^%l%$] %v");
    m_pLogger->set_level(spdlog::level::trace);
    spdlog::set_default_logger(m_pLogger); // Set as default.
}

///////////////////////////////////////////////////////////////////////////////
// Purpose: handles user input pre-init
// Input  : argc - 
//          *argv - 
// Output : exit_code (-1 if EntryPoint should continue to HandleInput)
///////////////////////////////////////////////////////////////////////////////
int CLauncher::HandleCommandLine(int argc, char* argv[])
{
    for (int i = 1; i < __argc; ++i)
    {
        std::string arg = __argv[i];
        eLaunchMode mode = eLaunchMode::LM_HOST;

        if ((arg == "-developer") || (arg == "-dev"))
        {
            mode = eLaunchMode::LM_HOST_DEV;
        }
        else if ((arg == "-retail") || (arg == "-prod"))
        {
            mode = eLaunchMode::LM_HOST;
        }
        else if ((arg == "-dedicated_dev") || (arg == "-dedid"))
        {
            mode = eLaunchMode::LM_SERVER_DEV;
        }
        else if ((arg == "-dedicated") || (arg == "-dedi"))
        {
            mode = eLaunchMode::LM_SERVER;
        }
        else if ((arg == "-client_dev") || (arg == "-cld"))
        {
            mode = eLaunchMode::LM_CLIENT_DEV;
        }
        else if ((arg == "-client") || (arg == "-cl"))
        {
            mode = eLaunchMode::LM_CLIENT;
        }

        if (CreateLaunchContext(mode) && LaunchProcess())
        {
            Sleep(2000);
            return EXIT_SUCCESS;
        }

        Sleep(2000);
        return EXIT_FAILURE;
    }
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
// Purpose: handles user input post-init
// Output : exit_code
///////////////////////////////////////////////////////////////////////////////
int CLauncher::HandleInput()
{
    std::cout << "----------------------------------------------------------------------------------------------------------------------" << std::endl;
    AddLog(spdlog::level::level_enum::warn, "If a DEV option has been chosen as launch parameter, do not broadcast servers to the Server Browser!\n");
    AddLog(spdlog::level::level_enum::warn, "All FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY ConVar's/ConCommand's will be enabled.\n");
    AddLog(spdlog::level::level_enum::warn, "Connected clients will be able to set and execute anything marked FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY.\n");
    std::cout << "----------------------------------------------------------------------------------------------------------------------" << std::endl;
    AddLog(spdlog::level::level_enum::warn, "Use DEV HOST          [0] for research and development purposes.\n");
    AddLog(spdlog::level::level_enum::warn, "Use RETAIL HOST       [1] for playing the game and creating servers.\n");
    AddLog(spdlog::level::level_enum::warn, "Use DEV SERVER        [2] for research and development purposes.\n");
    AddLog(spdlog::level::level_enum::warn, "Use RETAIL SERVER     [3] for running and hosting dedicated servers.\n");
    AddLog(spdlog::level::level_enum::warn, "Use DEV CLIENT        [4] for research and development purposes.\n");
    AddLog(spdlog::level::level_enum::warn, "Use RETAIL CLIENT     [5] for running the client only game.\n");
    std::cout << "----------------------------------------------------------------------------------------------------------------------" << std::endl;
    AddLog(spdlog::level::level_enum::info, "Enter '0' for 'DEV HOST'.\n");
    AddLog(spdlog::level::level_enum::info, "Enter '1' for 'RETAIL HOST'.\n");
    AddLog(spdlog::level::level_enum::info, "Enter '2' for 'DEV SERVER'.\n");
    AddLog(spdlog::level::level_enum::info, "Enter '3' for 'RETAIL SERVER'.\n");
    AddLog(spdlog::level::level_enum::info, "Enter '4' for 'DEV CLIENT'.\n");
    AddLog(spdlog::level::level_enum::info, "Enter '5' for 'RETAIL CLIENT'.\n");
    std::cout << "----------------------------------------------------------------------------------------------------------------------" << std::endl;
    std::cout << "User input: ";

    std::string input;
    if (std::cin >> input)
    {
        try
        {
            eLaunchMode mode = static_cast<eLaunchMode>(std::stoi(input));

            if (CreateLaunchContext(mode) && LaunchProcess())
            {
                Sleep(2000);
                return EXIT_SUCCESS;
            }
            else
            {
                AddLog(spdlog::level::level_enum::err, "Invalid mode (range 0-5).\n");
                Sleep(2000);
                return EXIT_FAILURE;
            }
        }
        catch (const std::exception& e)
        {
            AddLog(spdlog::level::level_enum::err, "SDK Launcher only takes numerical input; error: {:s}.\n", e.what());
            Sleep(2000);
            return EXIT_FAILURE;
        }
    }
    AddLog(spdlog::level::level_enum::err, "SDK Launcher requires numerical input.\n");

    Sleep(2000);
    return EXIT_FAILURE;
}

///////////////////////////////////////////////////////////////////////////////
// Purpose: create launch context.
// Input  : lMode - 
//          lState - 
//          *szCommandLine - 
// Output : true on success, false otherwise.
///////////////////////////////////////////////////////////////////////////////
bool CLauncher::CreateLaunchContext(eLaunchMode lMode, const char* szCommandLine /*= nullptr*/, const char* szConfig /*= nullptr*/)
{
    ///////////////////////////////////////////////////////////////////////////
    switch (lMode)
    {
    case eLaunchMode::LM_HOST_DEV:
    {
        if (!szConfig) { szConfig = "startup_dev.cfg"; }

        SetupLaunchContext(szConfig, MAIN_WORKER_DLL, MAIN_GAME_DLL, szCommandLine);
        AddLog(spdlog::level::level_enum::info, "*** LAUNCHING GAME [DEV] ***\n");
        break;
    }
    case eLaunchMode::LM_HOST:
    {
        if (!szConfig) { szConfig = "startup_retail.cfg"; }

        SetupLaunchContext(szConfig, MAIN_WORKER_DLL, MAIN_GAME_DLL, szCommandLine);
        AddLog(spdlog::level::level_enum::info, "*** LAUNCHING GAME [RETAIL] ***\n");
        break;
    }
    case eLaunchMode::LM_SERVER_DEV:
    {
        if (!szConfig) { szConfig = "startup_dedi_dev.cfg"; }

        SetupLaunchContext(szConfig, SERVER_WORKER_DLL, SERVER_GAME_DLL, szCommandLine);
        AddLog(spdlog::level::level_enum::info, "*** LAUNCHING DEDICATED [DEV] ***\n");
        break;
    }
    case eLaunchMode::LM_SERVER:
    {
        if (!szConfig) { szConfig = "startup_dedi_retail.cfg"; }

        SetupLaunchContext(szConfig, SERVER_WORKER_DLL, SERVER_GAME_DLL, szCommandLine);
        AddLog(spdlog::level::level_enum::info, "*** LAUNCHING DEDICATED [RETAIL] ***\n");
        break;
    }
    case eLaunchMode::LM_CLIENT_DEV:
    {
        if (!szConfig) { szConfig = "startup_client_dev.cfg"; }

        SetupLaunchContext(szConfig, CLIENT_WORKER_DLL, MAIN_GAME_DLL, szCommandLine);
        AddLog(spdlog::level::level_enum::info, "*** LAUNCHING CLIENT [DEV] ***\n");
        break;
    }
    case eLaunchMode::LM_CLIENT:
    {
        if (!szConfig) { szConfig = "startup_client_retail.cfg"; }

        SetupLaunchContext(szConfig, CLIENT_WORKER_DLL, MAIN_GAME_DLL, szCommandLine);
        AddLog(spdlog::level::level_enum::info, "*** LAUNCHING CLIENT [RETAIL] ***\n");
        break;
    }
    default:
    {
        AddLog(spdlog::level::level_enum::err, "*** NO LAUNCH MODE SPECIFIED ***\n");
        return false;
    }
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// Purpose: setup launch context.
// Input  : *szConfig      - 
//          *szWorkerDll   - 
//          *szGameDll     - 
//          *szCommandLine - 
///////////////////////////////////////////////////////////////////////////////
void CLauncher::SetupLaunchContext(const char* szConfig, const char* szWorkerDll, const char* szGameDll, const char* szCommandLine)
{
    CIOStream cfgFile;
    string commandLine;

    if (szConfig && szConfig[0])
    {
        if (cfgFile.Open(Format(GAME_CFG_PATH"%s", szConfig), CIOStream::READ))
        {
            if (!cfgFile.ReadString(commandLine))
            {
                AddLog(spdlog::level::level_enum::err, Format("Failed to read file '%s'!\n", szConfig));
            }
        }
        else // Failed to open config file.
        {
            AddLog(spdlog::level::level_enum::err, Format("Failed to open file '%s'!\n", szConfig));
        }
    }

    if (szCommandLine && szCommandLine[0])
    {
        commandLine.append(szCommandLine);
    }

    m_svWorkerDll = Format("%s\\%s", m_svCurrentDir.c_str(), szWorkerDll);
    m_svGameExe = Format("%s\\%s", m_svCurrentDir.c_str(), szGameDll);
    m_svCmdLine = Format("%s\\%s %s", m_svCurrentDir.c_str(), szGameDll, commandLine.c_str());

    ///////////////////////////////////////////////////////////////////////////
    // Print the file paths and arguments.
    std::cout << "----------------------------------------------------------------------------------------------------------------------" << std::endl;
    AddLog(spdlog::level::level_enum::debug, "- CWD: {:s}\n", m_svCurrentDir);
    AddLog(spdlog::level::level_enum::debug, "- EXE: {:s}\n", m_svGameExe);
    AddLog(spdlog::level::level_enum::debug, "- DLL: {:s}\n", m_svWorkerDll);
    AddLog(spdlog::level::level_enum::debug, "- CLI: {:s}\n", commandLine);
    std::cout << "----------------------------------------------------------------------------------------------------------------------" << std::endl;
}

///////////////////////////////////////////////////////////////////////////////
// Purpose: launches the game with results from the setup
// Output : true on success, false otherwise
///////////////////////////////////////////////////////////////////////////////
bool CLauncher::LaunchProcess() const
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
// EntryPoint.
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[], char* envp[])
{
    g_pLauncher->InitLogger();
    if (__argc < 2)
    {
        FreeConsole();
        g_pLauncher->RunSurface();
    }
    else
    {
        int results = g_pLauncher->HandleCommandLine(__argc, __argv);
        if (results != -1)
            return results;

        return g_pLauncher->HandleInput();
    }
    return EXIT_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// Singleton Launcher.
///////////////////////////////////////////////////////////////////////////////
CLauncher* g_pLauncher(new CLauncher("win_console"));
