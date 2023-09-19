//=============================================================================//
//
// Purpose: SDK launcher implementation.
//
//=============================================================================//
#include "tier0/binstream.h"
#include "basepanel.h"
#include "sdklauncher.h"

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
//          *argv[] - 
// Output : exit_code (-1 if EntryPoint should continue to HandleInput)
///////////////////////////////////////////////////////////////////////////////
int CLauncher::HandleCommandLine(int argc, char* argv[])
{
    for (int i = 1; i < argc; ++i)
    {
        string arg = argv[i];
        eLaunchMode mode = eLaunchMode::LM_NONE;

        if ((arg == "-developer") || (arg == "-dev"))
        {
            mode = eLaunchMode::LM_GAME_DEV;
        }
        else if ((arg == "-retail") || (arg == "-prod"))
        {
            mode = eLaunchMode::LM_GAME;
        }
        else if ((arg == "-server_dev") || (arg == "-svd"))
        {
            mode = eLaunchMode::LM_SERVER_DEV;
        }
        else if ((arg == "-server") || (arg == "-sv"))
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

        if (mode != eLaunchMode::LM_NONE)
        {
            if (CreateLaunchContext(mode) && LaunchProcess())
            {
                return EXIT_SUCCESS;
            }
        }
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
    AddLog(spdlog::level::level_enum::warn, "The '%s' options are for development purposes; use the '%s' options for default usage.\n", "DEV", "PROD");
    std::cout << "----------------------------------------------------------------------------------------------------------------------" << std::endl;
    AddLog(spdlog::level::level_enum::info, "%-6s ('0' = %s | '1' = %s).\n", "GAME", "DEV", "PROD");
    AddLog(spdlog::level::level_enum::info, "%-6s ('2' = %s | '3' = %s).\n", "SERVER", "DEV", "PROD");
    AddLog(spdlog::level::level_enum::info, "%-6s ('4' = %s | '5' = %s).\n", "CLIENT", "DEV", "PROD");
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
                return EXIT_SUCCESS;
            }
            else
            {
                AddLog(spdlog::level::level_enum::err, "Invalid mode (range 0-5).\n");
                return EXIT_FAILURE;
            }
        }
        catch (const std::exception& e)
        {
            AddLog(spdlog::level::level_enum::err, "SDK Launcher only takes numerical input (error = %s).\n", e.what());
            return EXIT_FAILURE;
        }
    }
    AddLog(spdlog::level::level_enum::err, "SDK Launcher requires numerical input.\n");
    return EXIT_FAILURE;
}

///////////////////////////////////////////////////////////////////////////////
// Purpose: create launch context.
// Input  : lMode - 
//          lState - 
//          *szCommandLine - 
// Output : true on success, false otherwise.
///////////////////////////////////////////////////////////////////////////////
bool CLauncher::CreateLaunchContext(eLaunchMode lMode, uint64_t nProcessorAffinity /*= NULL*/, const char* szCommandLine /*= nullptr*/, const char* szConfig /*= nullptr*/)
{
    ///////////////////////////////////////////////////////////////////////////
    const char* szGameDLL = nullptr;
    const char* szContext = nullptr;
    const char* szLevel = nullptr;

    std::function<void(const char*, const char*,
        const char*, const char*)> fnSetup = [&](
        const char* gameDLL, const char* context,
            const char* level, const char* config)
    {
        szGameDLL = gameDLL;
        szContext = context;
        szLevel = level;

        // Only set fall-back config
        // if not already set.
        if (!szConfig)
        {
            szConfig = config;
        }
    };

    m_ProcessorAffinity = nProcessorAffinity;

    switch (lMode)
    {
    case eLaunchMode::LM_GAME_DEV:
    {
        fnSetup(MAIN_GAME_DLL, "GAME",
            "DEV", "startup_dev.cfg");
        break;
    }
    case eLaunchMode::LM_GAME:
    {
        fnSetup(MAIN_GAME_DLL, "GAME",
            "PROD", "startup_retail.cfg");
        break;
    }
    case eLaunchMode::LM_SERVER_DEV:
    {
        fnSetup(SERVER_GAME_DLL, "SERVER",
            "DEV", "startup_dedi_dev.cfg");
        break;
    }
    case eLaunchMode::LM_SERVER:
    {
        fnSetup(SERVER_GAME_DLL, "SERVER",
            "PROD", "startup_dedi_retail.cfg");
        break;
    }
    case eLaunchMode::LM_CLIENT_DEV:
    {
        fnSetup(MAIN_GAME_DLL, "CLIENT",
            "DEV", "startup_client_dev.cfg");
        break;
    }
    case eLaunchMode::LM_CLIENT:
    {
        fnSetup(MAIN_GAME_DLL, "CLIENT",
            "PROD", "startup_client_retail.cfg");
        break;
    }
    default:
    {
        AddLog(spdlog::level::level_enum::err, "No launch mode specified.\n");
        return false;
    }
    }

    SetupLaunchContext(szConfig, szGameDLL, szCommandLine);
    AddLog(spdlog::level::level_enum::info, "*** LAUNCHING %s [%s] ***\n",
        szContext, szLevel);

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// Purpose: setup launch context.
// Input  : *szConfig      - 
//          *szWorkerDll   - 
//          *szGameDll     - 
//          *szCommandLine - 
///////////////////////////////////////////////////////////////////////////////
void CLauncher::SetupLaunchContext(const char* szConfig, const char* szGameDll, const char* szCommandLine)
{
    CIOStream cfgFile;
    string commandLine;

    if (szConfig && szConfig[0])
    {
        if (cfgFile.Open(Format(GAME_CFG_PATH"%s", szConfig), CIOStream::READ))
        {
            if (!cfgFile.ReadString(commandLine))
            {
                AddLog(spdlog::level::level_enum::err, "Failed to read file '%s'!\n", szConfig);
            }
        }
        else // Failed to open config file.
        {
            AddLog(spdlog::level::level_enum::err, "Failed to open file '%s'!\n", szConfig);
        }
    }

    if (szCommandLine && szCommandLine[0])
    {
        commandLine.append(szCommandLine);
    }

    m_svGameDll = Format("%s\\%s", m_svCurrentDir.c_str(), szGameDll);
    m_svCmdLine = Format("%s\\%s %s", m_svCurrentDir.c_str(), szGameDll, commandLine.c_str());

    ///////////////////////////////////////////////////////////////////////////
    // Print the file paths and arguments.
    std::cout << "----------------------------------------------------------------------------------------------------------------------" << std::endl;
    AddLog(spdlog::level::level_enum::debug, "- CWD: %s\n", m_svCurrentDir.c_str());
    AddLog(spdlog::level::level_enum::debug, "- EXE: %s\n", m_svGameDll.c_str());
    AddLog(spdlog::level::level_enum::debug, "- CLI: %s\n", commandLine.c_str());
    std::cout << "----------------------------------------------------------------------------------------------------------------------" << std::endl;
}

///////////////////////////////////////////////////////////////////////////////
// Purpose: launches the game with results from the setup
// Output : true on success, false otherwise
///////////////////////////////////////////////////////////////////////////////
bool CLauncher::LaunchProcess() const
{
    ///////////////////////////////////////////////////////////////////////////
    STARTUPINFOA StartupInfo = { 0 };
    PROCESS_INFORMATION ProcInfo = { 0 };

    // Initialize startup info struct.
    StartupInfo.cb = sizeof(STARTUPINFOA);


    ///////////////////////////////////////////////////////////////////////////
    // Create the game process in a suspended state with our dll.
    BOOL createResult = CreateProcessA(
        m_svGameDll.c_str(),                           // lpApplicationName
        (LPSTR)m_svCmdLine.c_str(),                    // lpCommandLine
        NULL,                                          // lpProcessAttributes
        NULL,                                          // lpThreadAttributes
        FALSE,                                         // bInheritHandles
        CREATE_SUSPENDED,                              // dwCreationFlags
        NULL,                                          // lpEnvironment
        m_svCurrentDir.c_str(),                        // lpCurrentDirectory
        &StartupInfo,                                  // lpStartupInfo
        &ProcInfo                                      // lpProcessInformation
    );

    ///////////////////////////////////////////////////////////////////////////
    // Failed to create the process.
    if (createResult)
    {
        if (m_ProcessorAffinity)
        {
            BOOL affinityResult = SetProcessAffinityMask(ProcInfo.hProcess, m_ProcessorAffinity);
            if (!affinityResult)
            {
                // Just print the result, don't return, as
                // the process was created successfully.
                PrintLastError();
            }
        }
    }
    else
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
// Purpose: Window enumerator callback.
// Input  : hwnd   -
//          lParam - 
// Output : TRUE on success, FALSE otherwise.
///////////////////////////////////////////////////////////////////////////////
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    vector<HWND>* pHandles = reinterpret_cast<vector<HWND>*>(lParam);
    if (!pHandles)
    {
        return FALSE;
    }

    char szClassName[256];
    if (!GetClassNameA(hwnd, szClassName, 256))
    {
        return FALSE;
    }

    if (strcmp(szClassName, DEFAULT_WINDOW_CLASS_NAME) == 0)
    {
        pHandles->push_back(hwnd);
    }
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// EntryPoint.
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[]/*, char* envp[]*/)
{
    g_pLauncher->InitLogger();
    if (argc < 2)
    {
#ifdef NDEBUG
        FreeConsole();
#endif // NDEBUG
        g_pLauncher->RunSurface();
    }
    else
    {
        int results = g_pLauncher->HandleCommandLine(argc, argv);
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
