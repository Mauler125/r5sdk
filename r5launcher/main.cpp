#include "pch.h"
#include "main.h"

//-----------------------------------------------------------------------------
// Print the error message to the console if any.
//-----------------------------------------------------------------------------
void PrintLastError()
{
    DWORD errorMessageID = ::GetLastError();
    if (errorMessageID != NULL)
    {
        LPSTR messageBuffer = nullptr;
        size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

        spdlog::error("{}", messageBuffer);
        LocalFree(messageBuffer);
    }
}

//-----------------------------------------------------------------------------
// Purpose case switch:
// * Launch the game in user specified mode and state.
// * Load specified command line arguments from a file on the disk.
// * Format the file paths for the game exe and specified hook dll.
//-----------------------------------------------------------------------------
bool LaunchR5Apex(LAUNCHMODE lMode, LAUNCHSTATE lState)
{
    ///////////////////////////////////////////////////////////////////////////
    // Initialize strings.
    std::string WorkerDll            = std::string();
    std::string GameDirectory        = std::string();
    std::string CommandLineArguments = std::string();
    std::string StartupCommandLine   = std::string();
    std::string currentDirectory     = std::filesystem::current_path().u8string();

    ///////////////////////////////////////////////////////////////////////////
    // Determine launch mode.
    switch (lMode)
    {
        case LAUNCHMODE::LM_DEBUG:
        {
            std::filesystem::path cfgPath = std::filesystem::current_path() /= "platform\\cfg\\startup_debug.cfg"; // Get cfg path for debug startup.
            std::ifstream cfgFile(cfgPath); // Read the cfg file.
            if (cfgFile.good() && cfgFile)  // Does the cfg file exist?
            {
                std::stringstream ss;
                ss << cfgFile.rdbuf();           // Read ifstream buffer into stringstream.
                CommandLineArguments = ss.str(); // Get all the contents of the cfg file.
            }
            else
            {
                spdlog::error("File 'platform\\cfg\\startup_debug.cfg' does not exist.\n");
                cfgFile.close();
                return false;
            }
            cfgFile.close(); // Close cfg file.

            WorkerDll           = currentDirectory + "\\r5dev.dll";                          // Get path to worker dll.
            GameDirectory       = currentDirectory + "\\r5apex.exe";                         // Get path to game executeable.
            StartupCommandLine  = currentDirectory + "\\r5apex.exe " + CommandLineArguments; // Setup startup command line string.

            spdlog::info("*** LAUNCHING GAME [DEBUG] ***\n");
            break;
        }
        case LAUNCHMODE::LM_RELEASE:
        {
            std::filesystem::path cfgPath = std::filesystem::current_path() /= "platform\\cfg\\startup_retail.cfg"; // Get cfg path for release startup.
            std::ifstream cfgFile(cfgPath); // Read the cfg file.
            if (cfgFile.good() && cfgFile)  // Does the cfg file exist?
            {
                std::stringstream ss;
                ss << cfgFile.rdbuf();           // Read ifstream buffer into stringstream.
                CommandLineArguments = ss.str(); // Get all the contents of the cfg file.
            }
            else
            {
                spdlog::error("File 'platform\\cfg\\startup_retail.cfg' does not exist.\n");
                cfgFile.close();
                return false;
            }
            cfgFile.close(); // Close cfg file.

            WorkerDll          = currentDirectory + "\\r5detours.dll";                      // Get path to worker dll.
            GameDirectory      = currentDirectory + "\\r5apex.exe";                         // Get path to game executeable.
            StartupCommandLine = currentDirectory + "\\r5apex.exe " + CommandLineArguments; // Setup startup command line string.

            spdlog::info("*** LAUNCHING GAME [RELEASE] ***\n");
            break;
        }
        case LAUNCHMODE::LM_DEDI:
        {
            std::filesystem::path cfgPath = std::filesystem::current_path() /= "platform\\cfg\\startup_dedi.cfg"; // Get cfg path for dedicated startup.
            std::ifstream cfgFile(cfgPath); // Read the cfg file.
            if (cfgFile.good() && cfgFile)  // Does the cfg file exist?
            {
                std::stringstream ss;
                ss << cfgFile.rdbuf();            // Read ifstream buffer into stringstream.
                CommandLineArguments = ss.str();  // Get all the contents of the cfg file.
            }
            else
            {
                spdlog::error("File 'platform\\cfg\\startup_dedi.cfg' does not exist.\n");
                cfgFile.close();
                return false;
            }
            cfgFile.close(); // Close cfg file.

            WorkerDll          = currentDirectory + "\\dedicated.dll";                      // Get path to worker dll.
            GameDirectory      = currentDirectory + "\\r5apex.exe";                         // Get path to game executeable.
            StartupCommandLine = currentDirectory + "\\r5apex.exe " + CommandLineArguments; // Setup startup command line string.

            spdlog::info("*** LAUNCHING GAME [DEDICATED] ***\n");
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
    std::cout << "--------------------------------------------------------------------------------------------------------" << std::endl;
    spdlog::debug("- CWD: {}\n", currentDirectory);
    spdlog::debug("- EXE: {}\n", GameDirectory);
    spdlog::debug("- DLL: {}\n", WorkerDll);
    spdlog::debug("- CLI: {}\n", CommandLineArguments);
    std::cout << "--------------------------------------------------------------------------------------------------------" << std::endl;

    ///////////////////////////////////////////////////////////////////////////
    // Build our list of dlls to inject.
    LPCSTR DllsToInject[1] =
    {
        WorkerDll.c_str()
    };

    STARTUPINFO StartupInfo = { 0 };
    PROCESS_INFORMATION ProcInfo = { 0 };

    // Initialize startup info struct.
    StartupInfo.cb = sizeof(STARTUPINFO);

    ///////////////////////////////////////////////////////////////////////////
    // Create the game process in a suspended state with our dll.
    BOOL result = DetourCreateProcessWithDllsA
    (
        GameDirectory.c_str(),                         // lpApplicationName
        (LPSTR)StartupCommandLine.c_str(),             // lpCommandLine
        NULL,                                          // lpProcessAttributes
        NULL,                                          // lpThreadAttributes
        FALSE,                                         // bInheritHandles
        CREATE_SUSPENDED,                              // dwCreationFlags
        NULL,                                          // lpEnvironment
        currentDirectory.c_str(),                      // lpCurrentDirectory
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

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if ((arg == "-debug") || (arg == "-dbg"))
        {
            LaunchR5Apex(LAUNCHMODE::LM_DEBUG, LAUNCHSTATE::LS_CHEATS);
            Sleep(2000);
            return EXIT_SUCCESS;
        }

        if ((arg == "-release") || (arg == "-rel"))
        {
            LaunchR5Apex(LAUNCHMODE::LM_RELEASE, LAUNCHSTATE::LS_CHEATS);
            Sleep(2000);
            return EXIT_SUCCESS;
        }

        if ((arg == "-dedicated") || (arg == "-dedi"))
        {
            LaunchR5Apex(LAUNCHMODE::LM_DEDI, LAUNCHSTATE::LS_CHEATS);
            Sleep(2000);
            return EXIT_SUCCESS;
        }
    }

    std::cout << "--------------------------------------------------------------------------------------------------------" << std::endl;
    spdlog::warn("If DEBUG has been choosen as launch parameter, do not broadcast servers to the Server Browser.\n");
    spdlog::warn("All FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY ConVar's/ConCommand's will be enabled.\n");
    spdlog::warn("Connected clients will be able to set and execute anything flagged FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY.\n");
    std::cout << "--------------------------------------------------------------------------------------------------------" << std::endl;
    spdlog::warn("Use DEBUG     [1] for development and debugging purposes.\n");
    spdlog::warn("Use RELEASE   [2] for normal playing and server hosting.\n");
    spdlog::warn("Use DEDICATED [3] for running a dedicated server.\n");
    std::cout << "--------------------------------------------------------------------------------------------------------" << std::endl;
    spdlog::info("Enter 1 for DEBUG. Enter 2 for RELEASE. Enter 3 for DEDICATED: ");

    std::string input = std::string();
    if (std::cin >> input)
    {
        try
        {
            LAUNCHMODE iinput = (LAUNCHMODE)std::stoi(input);
            switch (iinput)
            {
            case LAUNCHMODE::LM_DEBUG:
            {
                LaunchR5Apex(LAUNCHMODE::LM_DEBUG, LAUNCHSTATE::LS_CHEATS);
                Sleep(2000);
                return EXIT_SUCCESS;
            }
            case LAUNCHMODE::LM_RELEASE:
            {
                LaunchR5Apex(LAUNCHMODE::LM_RELEASE, LAUNCHSTATE::LS_CHEATS);
                Sleep(2000);
                return EXIT_SUCCESS;
            }
            case LAUNCHMODE::LM_DEDI:
            {
                LaunchR5Apex(LAUNCHMODE::LM_DEDI, LAUNCHSTATE::LS_CHEATS);
                Sleep(2000);
                return EXIT_SUCCESS;
            }
            default:
            {
                spdlog::error("R5Reloaded requires '1' for DEBUG mode, '2' for RELEASE mode, '3' for DEDICATED mode.\n");
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