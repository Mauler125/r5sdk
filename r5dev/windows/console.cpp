#include "core/stdafx.h"
#include "core/init.h"
#include "rtech/rtech_utils.h"
#ifndef DEDICATED
#include "windows/id3dx.h"
#endif // !DEDICATED
#include "windows/console.h"
#include "client/IVEngineClient.h"
#include "common/opcodes.h"

//#############################################################################
// INITIALIZATION
//#############################################################################

void Console_Init()
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
	freopen_s(&fDummy, "CONIN$", "r", stdin);
	freopen_s(&fDummy, "CONOUT$", "w", stdout);
	freopen_s(&fDummy, "CONOUT$", "w", stderr);

	///////////////////////////////////////////////////////////////////////////
	// Create a worker thread to process console commands
	DWORD threadId;
	DWORD __stdcall ProcessConsoleWorker(LPVOID);
	HANDLE hThread = CreateThread(NULL, 0, ProcessConsoleWorker, NULL, 0, &threadId);

	// Initialize global spdlog.
	auto console = spdlog::stdout_logger_mt("console");
	console->set_pattern("[%S.%e] %v"); // Set pattern.

	spdlog::set_level(spdlog::level::trace);
	spdlog::set_default_logger(console); // Set as default.
	spdlog::flush_every(std::chrono::seconds(5)); // Flush buffers every 5 seconds for every logger.
	
	if (hThread)
	{
		spdlog::debug("THREAD ID: {}\n\n", threadId);
		CloseHandle(hThread);
	}
}

//#############################################################################
// WORKER THREAD
//#############################################################################

DWORD __stdcall ProcessConsoleWorker(LPVOID)
{
	// Loop forever
	while (true)
	{
		static std::string sCommand = "";

		///////////////////////////////////////////////////////////////////////
		// Get the user input on the debug console
		printf(">");
		std::getline(std::cin, sCommand);

		///////////////////////////////////////////////////////////////////////
		// Debug toggles
		if (sCommand == "pattern test") { PrintHAddress(); PrintOAddress(); continue; }
		if (sCommand == "opcodes test") { RuntimePtc_Toggle(); continue; }
		///////////////////////////////////////////////////////////////////////
		// Execute the command in the r5 SQVM
		IVEngineClient_CommandExecute(NULL, sCommand.c_str());
		sCommand.clear();

		///////////////////////////////////////////////////////////////////////
		// Sleep and loop
		Sleep(50);
	}
	return NULL;
}
