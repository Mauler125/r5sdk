#include "pch.h"
#include "id3dx.h"
#include "hooks.h"
#include "opcptc.h"
#include "console.h"
#include "patterns.h"

//#############################################################################
// INITIALIZATION
//#############################################################################

void SetupConsole()
{
	///////////////////////////////////////////////////////////////////////////
	// Create the console window
	if (AllocConsole() == FALSE)
	{
		OutputDebugString("Failed to create console window!\n");
		return;
	}

	///////////////////////////////////////////////////////////////////////////
	// Set the window title
	FILE* sBuildTxt;
	CHAR sBuildBuf[1024] = { 0 };

	fopen_s(&sBuildTxt, "build.txt", "r");
	if (sBuildTxt)
	{
		while (fgets(sBuildBuf, sizeof(sBuildBuf), sBuildTxt) != NULL)
		{
			fclose(sBuildTxt);
		}
	}
	SetConsoleTitle(sBuildBuf);

	///////////////////////////////////////////////////////////////////////////
	// Open input/output streams
	FILE* fDummy;
	freopen_s(&fDummy, "CONIN$",  "r",  stdin);
	freopen_s(&fDummy, "CONOUT$", "w", stdout);
	freopen_s(&fDummy, "CONOUT$", "w", stderr);

	///////////////////////////////////////////////////////////////////////////
	// Create a worker thread to process console commands
	DWORD threadId0;
	DWORD __stdcall ProcessConsoleWorker(LPVOID);
	HANDLE hThread0 = CreateThread(NULL, 0, ProcessConsoleWorker, NULL, 0, &threadId0);

	if (hThread0)
	{
		printf("THREAD ID: %ld\n\n", threadId0);
		CloseHandle(hThread0);
	}

	// Initialize global spdlog.
	auto console = spdlog::stdout_logger_mt("console");
	console->set_pattern("[%I:%M:%S:%e] [%L] %v"); // Set pattern.
	spdlog::set_default_logger(console); // Set as default.
	spdlog::flush_every(std::chrono::seconds(5)); // Flush buffers every 5 seconds for every logger.
#ifdef _DEBUG
	console->set_level(spdlog::level::debug);
#endif
	spdlog::debug("Console and spdlog are setup now!\n");
}

//#############################################################################
// WORKER THREAD
//#############################################################################

DWORD __stdcall ProcessConsoleWorker(LPVOID)
{
	while (true) // Loop forever
	{
		std::string sCommand;

		///////////////////////////////////////////////////////////////////////
		// Get the user input on the debug console
		printf(">");
		std::getline(std::cin, sCommand);

		///////////////////////////////////////////////////////////////////////
		// Engine toggles
		if (sCommand == "toggle net") { Hooks::ToggleNetTrace(); continue; }
		if (sCommand == "toggle dev") { Hooks::ToggleDevCommands(); continue; }
		if (sCommand == "toggle fal") { g_bReturnAllFalse = !g_bReturnAllFalse; continue; }
		///////////////////////////////////////////////////////////////////////
		// Debug toggles
		if (sCommand == "pattern test") { PrintHAddress(); PrintOAddress(); continue; }
		if (sCommand == "directx test") { PrintDXAddress(); continue; }
		if (sCommand == "console test") { g_bDebugConsole = !g_bDebugConsole; continue; }
		///////////////////////////////////////////////////////////////////////
		// Exec toggles
		if (sCommand == "1") { Hooks::ToggleDevCommands(); addr_CommandExecute(NULL, "exec autoexec_dev"); }
		if (sCommand == "2") { g_bDebugLoading = !g_bDebugLoading; continue; }

		///////////////////////////////////////////////////////////////////////
		// Execute the command in the r5 SQVM
		addr_CommandExecute(NULL, sCommand.c_str());
		sCommand.clear();

		///////////////////////////////////////////////////////////////////////
		// Sleep and loop
		Sleep(50);
	}

	return 0;
}