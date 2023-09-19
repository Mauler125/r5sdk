//=============================================================================//
//
// Purpose: Windows terminal utilities
//
//=============================================================================//

#include "core/stdafx.h"
#ifndef NETCONSOLE
#include "core/init.h"
#include "core/logdef.h"
#include "tier0/frametask.h"
#include "engine/cmd.h"
#ifndef DEDICATED
#include "windows/id3dx.h"
#endif // !DEDICATED
#endif // !NETCONSOLE
#include "windows/system.h"
#include "windows/console.h"

static std::string s_ConsoleInput;

//-----------------------------------------------------------------------------
// Purpose: sets the windows terminal background color
// Input  : color - 
//-----------------------------------------------------------------------------
void SetConsoleBackgroundColor(COLORREF color)
{
	CONSOLE_SCREEN_BUFFER_INFOEX sbInfoEx{0};
	sbInfoEx.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);

	HANDLE consoleOut = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfoEx(consoleOut, &sbInfoEx);

	// The +='' 1 is required, else the window will shrink
	// by '1' column and row each time this function is
	// getting called on the same console window. The
	// lower right bounds are detected inclusively on the
	// 'GetConsoleScreenBufferEx' call and exclusively
	// on the 'SetConsoleScreenBufferEx' call.
	sbInfoEx.srWindow.Right += 1;
	sbInfoEx.srWindow.Bottom += 1;

	sbInfoEx.ColorTable[0] = color;
	SetConsoleScreenBufferInfoEx(consoleOut, &sbInfoEx);
}

//-----------------------------------------------------------------------------
// Purpose: flashes the windows terminal background color
// Input  : nFlashCount - 
//			nFlashInterval -
//			color -
//-----------------------------------------------------------------------------
void FlashConsoleBackground(int nFlashCount, int nFlashInterval, COLORREF color)
{
	CONSOLE_SCREEN_BUFFER_INFOEX sbInfoEx{0};
	sbInfoEx.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);

	HANDLE consoleOut = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfoEx(consoleOut, &sbInfoEx);

	COLORREF storedBG = sbInfoEx.ColorTable[0];

	for (int i = 0; i < nFlashCount; ++i)
	{
		//-- set BG color
		Sleep(nFlashInterval);
		sbInfoEx.ColorTable[0] = color;
		SetConsoleScreenBufferInfoEx(consoleOut, &sbInfoEx);

		//-- restore previous color
		Sleep(nFlashInterval);
		sbInfoEx.ColorTable[0] = storedBG;
		SetConsoleScreenBufferInfoEx(consoleOut, &sbInfoEx);
	}
}

//-----------------------------------------------------------------------------
// Purpose: terminal window setup
// Input  : bAnsiColor - 
//-----------------------------------------------------------------------------
void Console_Init(const bool bAnsiColor)
{
#ifndef NETCONSOLE
	///////////////////////////////////////////////////////////////////////////
	// Create the console window
	if (AllocConsole() == FALSE)
	{
		OutputDebugStringA("Failed to create console window!\n");
		return;
	}

	//-- Set the window title
	SetConsoleTitleA("R5");

	//-- Open input/output streams
	FILE* fDummy;
	freopen_s(&fDummy, "CONIN$", "r", stdin);
	freopen_s(&fDummy, "CONOUT$", "w", stdout);
	freopen_s(&fDummy, "CONOUT$", "w", stderr);

	//-- Create a worker thread to process console commands
	DWORD dwThreadId = NULL;
	DWORD __stdcall ProcessConsoleWorker(LPVOID);
	HANDLE hThread = CreateThread(NULL, 0, ProcessConsoleWorker, NULL, 0, &dwThreadId);
	
	if (hThread)
	{
		CloseHandle(hThread);
	}
#endif // !NETCONSOLE

	HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD dwMode = NULL;

	if (bAnsiColor)
	{
		GetConsoleMode(hOutput, &dwMode);
		dwMode |= ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;

		if (!SetConsoleMode(hOutput, dwMode)) // Some editions of Windows have 'VirtualTerminalLevel' disabled by default.
		{
			// Warn the user if 'VirtualTerminalLevel' could not be set on users environment.
			MessageBoxA(NULL, "Failed to set console mode 'VirtualTerminalLevel'.\n"
				"Please omit the '-ansicolor' parameter and restart \nthe program if output logging appears distorted.", "SDK Warning", MB_ICONEXCLAMATION | MB_OK);
		}

		SetConsoleBackgroundColor(0x00000000);
		AnsiColors_Init();
	}

#ifndef NETCONSOLE
	SetConsoleCtrlHandler(ConsoleHandlerRoutine, true);
#endif // !NETCONSOLE
}

//-----------------------------------------------------------------------------
// Purpose: terminal window shutdown
//-----------------------------------------------------------------------------
void Console_Shutdown()
{
	///////////////////////////////////////////////////////////////////////////
	// Destroy the console window
	if (FreeConsole() == FALSE)
	{
		OutputDebugStringA("Failed to destroy console window!\n");
		return;
	}
}

#ifndef NETCONSOLE
//#############################################################################
// CONSOLE WORKER
//#############################################################################
DWORD __stdcall ProcessConsoleWorker(LPVOID)
{
	while (true)
	{
		//printf("] ");
		//-- Get the user input on the debug console
		std::getline(std::cin, s_ConsoleInput);

		// Execute the command.
		Cbuf_AddText(Cbuf_GetCurrentPlayer(), s_ConsoleInput.c_str(), cmd_source_t::kCommandSrcCode);

		if (!s_ConsoleInput.empty())
			s_ConsoleInput.clear();

		Sleep(50);
	}
	return NULL;
}
#endif // !NETCONSOLE
