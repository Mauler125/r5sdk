//=============================================================================//
//
// Purpose: Windows terminal utilities
//
//=============================================================================//

#include "core/stdafx.h"
#ifndef _TOOLS
#include "core/init.h"
#include "core/logdef.h"
#include "tier0/frametask.h"
#include "engine/cmd.h"
#ifndef DEDICATED
#include "windows/id3dx.h"
#endif // !DEDICATED
#endif // !_TOOLS
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
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool Console_Init(const bool bAnsiColor)
{
	char msgBuf[2048];

	///////////////////////////////////////////////////////////////////////////
	// Create the console window
	if (AllocConsole() == FALSE)
	{
		snprintf(msgBuf, sizeof(msgBuf), "Failed to create console window! [%s]\n", 
			std::system_category().message(static_cast<int>(::GetLastError())).c_str());

		OutputDebugStringA(msgBuf);
		return false;
	}

#ifndef _TOOLS
	//-- Set the window title
	SetConsoleTitleA("R5");
#endif // !_TOOLS

	//-- Open input/output streams
	FILE* fDummy;
	freopen_s(&fDummy, "CONIN$", "r", stdin);
	freopen_s(&fDummy, "CONOUT$", "w", stdout);
	freopen_s(&fDummy, "CONOUT$", "w", stderr);

#ifndef _TOOLS
	//-- Create a worker thread to process console commands
	DWORD dwThreadId = NULL;
	DWORD __stdcall ProcessConsoleWorker(LPVOID);
	HANDLE hThread = CreateThread(NULL, 0, ProcessConsoleWorker, NULL, 0, &dwThreadId);
	
	if (hThread)
	{
		CloseHandle(hThread);
	}
#endif // !_TOOLS

	if (bAnsiColor)
	{
		if (!Console_ColorInit())
		{
			Assert(0);
			//snprintf(msgBuf, sizeof(msgBuf), "Failed to set color console mode! [%s]\n",
			//	std::system_category().message(static_cast<int>(::GetLastError())).c_str());

			//MessageBoxA(NULL, msgBuf, "SDK Warning", MB_ICONEXCLAMATION | MB_OK);
		}
	}

#ifndef _TOOLS
	SetConsoleCtrlHandler(ConsoleHandlerRoutine, true);
#endif // !_TOOLS

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: terminal color setup
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool Console_ColorInit()
{
	HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD dwMode = NULL;

	GetConsoleMode(hOutput, &dwMode); // Some editions of Windows have 'VirtualTerminalLevel' disabled by default.
	dwMode |= ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;

	if (!SetConsoleMode(hOutput, dwMode))
		return false; // Failure.

	SetConsoleBackgroundColor(0x00000000);
	AnsiColors_Init();

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: terminal window shutdown
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool Console_Shutdown()
{
	///////////////////////////////////////////////////////////////////////////
	// Destroy the console window
	if (FreeConsole() == FALSE)
	{
		char szBuf[2048];
		snprintf(szBuf, sizeof(szBuf), "Failed to destroy console window! [%s]\n", 
			std::system_category().message(static_cast<int>(::GetLastError())).c_str());

		OutputDebugStringA(szBuf);
		return false;
	}

	return true;
}

#ifndef _TOOLS
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
#endif // !_TOOLS
