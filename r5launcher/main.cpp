#include <string>
#include <stdio.h>
#include <iostream>
#include <Windows.h>
#include <detours.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include "main.h"

 //----------------------------------------------------------------------------
 // Print the error message to the console if any
 //----------------------------------------------------------------------------
void PrintLastError()
{
    DWORD errorMessageID = ::GetLastError();
    if (errorMessageID != NULL)
    {
        LPSTR messageBuffer = nullptr;
        size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

        std::cout << "ERROR: " << messageBuffer << std::endl;
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
        case LAUNCHMODE::LM_DEDI:
        {
            std::filesystem::path cfgPath = std::filesystem::current_path() /= "platform\\cfg\\startup_dedi.cfg"; // Get cfg path for dedicated startup.
            std::ifstream cfgFile(cfgPath); // Read the cfg file.
            if (cfgFile.good() && cfgFile) // Does the cfg file exist?
            {
                std::stringstream ss;
                ss << cfgFile.rdbuf(); // Read ifstream buffer into stringstream.
                CommandLineArguments = ss.str();  // Get all the contents of the cfg file.
            }
            else
            {
                std::cout << "*** platform\\cfg\\startup_dedi.cfg does not exist. ***" << std::endl;
                cfgFile.close();
                return false;
            }
            cfgFile.close(); // Close cfg file.

            WorkerDll          = currentDirectory + "\\dedicated.dll";                      // Get path to worker dll.
            GameDirectory      = currentDirectory + "\\r5apex.exe";                         // Get path to game executeable.
            StartupCommandLine = currentDirectory + "\\r5apex.exe " + CommandLineArguments; // Setup startup command line string.

            std::cout << "*** LAUNCHING GAME [DEDICATED] ***" << std::endl;
            break;
        }
        case LAUNCHMODE::LM_DEBUG:
        {
            std::filesystem::path cfgPath = std::filesystem::current_path() /= "platform\\cfg\\startup_debug.cfg"; // Get cfg path for debug startup.
            std::ifstream cfgFile(cfgPath); // Read the cfg file.
            if (cfgFile.good() && cfgFile) // Does the cfg file exist?
            {
                std::stringstream ss;
                ss << cfgFile.rdbuf(); // Read ifstream buffer into stringstream.
                CommandLineArguments = ss.str();  // Get all the contents of the cfg file.
            }
            else
            {
                std::cout << "*** platform\\cfg\\startup_debug.cfg does not exist. ***" << std::endl;
                cfgFile.close();
                return false;
            }
            cfgFile.close(); // Close cfg file.

            WorkerDll           = currentDirectory + "\\r5dev.dll";                          // Get path to worker dll.
            GameDirectory       = currentDirectory + "\\r5apex.exe";                         // Get path to game executeable.
            StartupCommandLine  = currentDirectory + "\\r5apex.exe " + CommandLineArguments; // Setup startup command line string.

            std::cout << "*** LAUNCHING GAME [DEBUG] ***" << std::endl;
            break;
        }
        case LAUNCHMODE::LM_GAME:
        {
            std::filesystem::path cfgPath = std::filesystem::current_path() /= "platform\\cfg\\startup_retail.cfg"; // Get cfg path for release startup.
            std::ifstream cfgFile(cfgPath); // Read the cfg file.
            if (cfgFile.good() && cfgFile) // Does the cfg file exist?
            {
                std::stringstream ss;
                ss << cfgFile.rdbuf(); // Read ifstream buffer into stringstream.
                CommandLineArguments = ss.str();  // Get all the contents of the cfg file.
            }
            else
            {
                std::cout << "*** platform\\cfg\\startup_retail.cfg does not exist. ***" << std::endl;
                cfgFile.close();
                return false;
            }
            cfgFile.close(); // Close cfg file.

            WorkerDll          = currentDirectory + "\\r5detours.dll";                      // Get path to worker dll.
            GameDirectory      = currentDirectory + "\\r5apex.exe";                         // Get path to game executeable.
            StartupCommandLine = currentDirectory + "\\r5apex.exe " + CommandLineArguments; // Setup startup command line string.

            std::cout << "*** LAUNCHING GAME [RELEASE] ***" << std::endl;
            break;
        }
        default:
        {
            std::cout << "*** ERROR: NO LAUNCHMODE SPECIFIED ***" << std::endl;
            return false;
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    // Print the filepaths and arguments.
    std::cout << " - CWD: " << currentDirectory     << std::endl;
    std::cout << " - EXE: " << GameDirectory        << std::endl;
    std::cout << " - DLL: " << WorkerDll            << std::endl;
    std::cout << " - CLI: " << CommandLineArguments << std::endl;

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
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if ((arg == "-dedicated") || (arg == "-dedi"))
        {
            LaunchR5Apex(LAUNCHMODE::LM_DEDI, LAUNCHSTATE::LS_CHEATS);
            Sleep(2000);
            return EXIT_SUCCESS;
        }

        if ((arg == "-debug") || (arg == "-dbg"))
        {
            LaunchR5Apex(LAUNCHMODE::LM_DEBUG, LAUNCHSTATE::LS_CHEATS);
            Sleep(2000);
            return EXIT_SUCCESS;
        }

        if ((arg == "-release") || (arg == "-rl"))
        {
            LaunchR5Apex(LAUNCHMODE::LM_GAME, LAUNCHSTATE::LS_CHEATS);
            Sleep(2000);
            return EXIT_SUCCESS;
        }
    }

    std::cout << "If you choose Dev as start parameter do not host servers into the Server Browser\n\n" 
        << "Every command will be and people can execute any script on your server.\n\n"
        << "Use release for normal playing.\n\n"
        << "Dev should only be used for testing purposes.\n\n";

    std::cout << "Enter 1 for Dev Build. Enter 2 for Release Build:\n";

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
            case LAUNCHMODE::LM_GAME:
            {
                LaunchR5Apex(LAUNCHMODE::LM_GAME, LAUNCHSTATE::LS_CHEATS);
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
                std::cout << "R5 Reloaded asked for a number between 1 and 2 :(.\n";
                Sleep(2000);
                return EXIT_FAILURE;
            }
            }
        }
        catch (std::exception& e)
        {
            std::cout << "R5 Reloaded asked for a number and not a letter or anything of that sort :(. Error: " << e.what() << std::endl;
            Sleep(5000);
            return EXIT_FAILURE;
        }
    }

    std::cout << "R5 Reloaded needs an input to launch :(.\n";

    Sleep(5000);

    return EXIT_FAILURE;
}