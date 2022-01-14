#include "core/stdafx.h"
#include "core/init.h"
#include "core/logdef.h"
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
	DWORD dwMode = NULL;
	DWORD dwThreadId = NULL;
	DWORD __stdcall ProcessConsoleWorker(LPVOID);
	HANDLE hThread = CreateThread(NULL, 0, ProcessConsoleWorker, NULL, 0, &dwThreadId);

	//HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	
	if (hThread)
	{
		CloseHandle(hThread);
	}

	if (strstr(GetCommandLineA(), "-ansiclr"))
	{
		GetConsoleMode(hOutput, &dwMode);
		dwMode |= ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;

		if (!SetConsoleMode(hOutput, dwMode)) // Some editions of Windows have 'VirtualTerminalLevel' disabled by default.
		{
			// Warn the user if 'VirtualTerminalLevel' could not be set on users environment.
			MessageBox(NULL, "Failed to set console mode 'VirtualTerminalLevel'.\nPlease omit the '-ansiclr' parameter and restart\nthe game if output logging appears distorted.", "SDK Warning", MB_ICONEXCLAMATION | MB_OK);
		}
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
		if (sCommand == "pattern test") { PrintHAddress(); continue; }
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
