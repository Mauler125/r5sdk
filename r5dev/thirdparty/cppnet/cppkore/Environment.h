#pragma once

#include "ListBase.h"
#include "StringBase.h"
#include "SpecialFolder.h"
#include <cstdint>

#undef GetCommandLine
#undef SetEnvironmentVariable
#undef GetEnvironmentVariable
#undef GetComputerName
#undef GetUserName

namespace System
{
	class Environment
	{
	public:
		// Terminates the process with the given code
		static void Exit(int32_t ExitCode);

		// Gets the path to the specific system file folder
		static string GetFolderPath(SpecialFolder Folder);
		// Returns the application path
		static string GetApplicationPath();
		// Returns the application
		static string GetApplication();
		// Rethrns the full command line string
		static string GetCommandLine();
		// Returns a list of command line arguments
		static List<string> GetCommandLineArgs();
		// Returns the current users account name
		static string GetUserName();
		// Returns the current computer name
		static string GetComputerName();
		// Returns the total tick count since startup
		static uint64_t GetTickCount();

		// Sets an environment variable to the specified value
		static void SetEnvironmentVariable(const string& Key, const string& Value);
		// Gets an environment variable from the specified key
		static string GetEnvironmentVariable(const string& Key);
		// Expands environment variables in the given path
		static string ExpandEnvironmentVariables(const string& Path);

		// Returns whether or not we are a 64bit process
		constexpr static bool Is64BitProcess();

		// Returns a newline string for the environment
		const static string NewLine;
	};
}