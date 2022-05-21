#include "stdafx.h"
#include "Console.h"
#include "ConsoleStream.h"

namespace System
{
	// This holds the global std handle for the input stream
	__ConsoleInit Console::ConsoleInstance = __ConsoleInit();

	enum class ControlKeyState
	{
		RightAltPressed = 0x0001,
		LeftAltPressed = 0x0002,
		RightCtrlPressed = 0x0004,
		LeftCtrlPressed = 0x0008,
		ShiftPressed = 0x0010,
		NumLockOn = 0x0020,
		ScrollLockOn = 0x0040,
		CapsLockOn = 0x0080,
		EnhancedKey = 0x0100
	};

	void Console::Beep()
	{
		::Beep(800, 200);
	}

	void Console::Beep(uint32_t Frequency, uint32_t Duration)
	{
		::Beep(Frequency, Duration);
	}

	void Console::Clear()
	{
		auto hStdOut = ((IO::ConsoleStream*)ConsoleInstance.Out.GetBaseStream())->GetStreamHandle();

		CONSOLE_SCREEN_BUFFER_INFO cSBI{};
		GetConsoleScreenBufferInfo(hStdOut, &cSBI);

		COORD cScreen{};
		int conSize = (cSBI.dwSize.X * cSBI.dwSize.Y);

		DWORD nCellsWritten = 0;
		FillConsoleOutputCharacterA(hStdOut, ' ', conSize, cScreen, &nCellsWritten);

		nCellsWritten = 0;
		FillConsoleOutputAttribute(hStdOut, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED, conSize, cScreen, &nCellsWritten);

		SetConsoleCursorPosition(hStdOut, cScreen);
	}

	void Console::ClearLine()
	{
		auto hStdOut = ((IO::ConsoleStream*)ConsoleInstance.Out.GetBaseStream())->GetStreamHandle();

		CONSOLE_SCREEN_BUFFER_INFO cSBI{};
		GetConsoleScreenBufferInfo(hStdOut, &cSBI);

		cSBI.dwCursorPosition.X = 0;

		DWORD nCellsWritten = 0;
		FillConsoleOutputCharacterA(hStdOut, ' ', cSBI.dwSize.X, cSBI.dwCursorPosition, &nCellsWritten);

		SetConsoleCursorPosition(hStdOut, cSBI.dwCursorPosition);
	}

	void Console::ResetColor()
	{
		Console::SetColors(ConsoleColor::Gray, ConsoleColor::Black);
	}

	void Console::SetForegroundColor(ConsoleColor Color)
	{
		auto hStdOut = ((IO::ConsoleStream*)ConsoleInstance.Out.GetBaseStream())->GetStreamHandle();

		CONSOLE_SCREEN_BUFFER_INFO cSBI{};
		GetConsoleScreenBufferInfo(hStdOut, &cSBI);

		auto nColor = Console::ConsoleColorToColorAttribute(Color, false);

		int16_t Attrs = cSBI.wAttributes;

		Attrs &= ~((int16_t)Console::ForegroundMask);
		Attrs = (int16_t)(((uint32_t)(uint16_t)Attrs) | ((uint32_t)(uint16_t)nColor));

		SetConsoleTextAttribute(hStdOut, Attrs);
	}

	void Console::SetBackgroundColor(ConsoleColor Color)
	{
		auto hStdOut = ((IO::ConsoleStream*)ConsoleInstance.Out.GetBaseStream())->GetStreamHandle();

		CONSOLE_SCREEN_BUFFER_INFO cSBI{};
		GetConsoleScreenBufferInfo(hStdOut, &cSBI);

		auto nColor = Console::ConsoleColorToColorAttribute(Color, true);

		int16_t Attrs = cSBI.wAttributes;

		Attrs &= ~((int16_t)Console::BackgroundMask);
		Attrs = (int16_t)(((uint32_t)(uint16_t)Attrs) | ((uint32_t)(uint16_t)nColor));

		SetConsoleTextAttribute(hStdOut, Attrs);
	}

	void Console::SetColors(ConsoleColor Foreground, ConsoleColor Background)
	{
		auto hStdOut = ((IO::ConsoleStream*)ConsoleInstance.Out.GetBaseStream())->GetStreamHandle();

		CONSOLE_SCREEN_BUFFER_INFO cSBI{};
		GetConsoleScreenBufferInfo(hStdOut, &cSBI);

		auto nColor = Console::ConsoleColorToColorAttribute(Foreground, false);
		auto nColor2 = Console::ConsoleColorToColorAttribute(Background, true);

		int16_t Attrs = cSBI.wAttributes;

		Attrs &= ~((int16_t)Console::ForegroundMask);
		Attrs = (int16_t)(((uint32_t)(uint16_t)Attrs) | ((uint32_t)(uint16_t)nColor));

		Attrs &= ~((int16_t)Console::BackgroundMask);
		Attrs = (int16_t)(((uint32_t)(uint16_t)Attrs) | ((uint32_t)(uint16_t)nColor2));

		SetConsoleTextAttribute(hStdOut, Attrs);
	}

	void Console::SetBufferSize(uint32_t Width, uint32_t Height)
	{
		COORD cScreen;
		cScreen.X = (SHORT)Width;
		cScreen.Y = (SHORT)Height;

		SetConsoleScreenBufferSize(((IO::ConsoleStream*)ConsoleInstance.Out.GetBaseStream())->GetStreamHandle(), cScreen);
	}

	void Console::SetCursorPosition(uint32_t Left, uint32_t Right)
	{
		auto hStdOut = ((IO::ConsoleStream*)ConsoleInstance.Out.GetBaseStream())->GetStreamHandle();

		COORD cScreen;
		cScreen.X = (SHORT)Left;
		cScreen.Y = (SHORT)Right;

		SetConsoleCursorPosition(hStdOut, cScreen);
	}

	void Console::SetWindowSize(uint32_t Width, uint32_t Height)
	{
		auto hStdOut = ((IO::ConsoleStream*)ConsoleInstance.Out.GetBaseStream())->GetStreamHandle();

		CONSOLE_SCREEN_BUFFER_INFO cSBI{};
		GetConsoleScreenBufferInfo(hStdOut, &cSBI);

		COORD cScreen;
		cScreen.X = cSBI.dwSize.X;
		cScreen.Y = cSBI.dwSize.Y;

		bool nBufferResize = false;
		if (cSBI.dwSize.X < (SHORT)(cSBI.srWindow.Left + Width))
		{
			cScreen.X = (SHORT)(cSBI.srWindow.Left + Width);
			nBufferResize = true;
		}
		if (cSBI.dwSize.Y < (SHORT)(cSBI.srWindow.Top + Height))
		{
			cScreen.Y = (SHORT)(cSBI.srWindow.Top + Height);
			nBufferResize = true;
		}

		if (nBufferResize)
			SetConsoleScreenBufferSize(hStdOut, cScreen);

		SMALL_RECT srWindow = cSBI.srWindow;
		srWindow.Bottom = (SHORT)(srWindow.Top + Height - 1);
		srWindow.Right = (SHORT)(srWindow.Left + Width - 1);

		auto hResult = SetConsoleWindowInfo(hStdOut, true, &srWindow);
		if (!hResult)
			SetConsoleScreenBufferSize(hStdOut, cSBI.dwSize);
	}

	void Console::SetWindowPosition(uint32_t Left, uint32_t Top)
	{
		auto hStdOut = ((IO::ConsoleStream*)ConsoleInstance.Out.GetBaseStream())->GetStreamHandle();

		CONSOLE_SCREEN_BUFFER_INFO cSBI{};
		GetConsoleScreenBufferInfo(hStdOut, &cSBI);

		SMALL_RECT srWindow = cSBI.srWindow;

		srWindow.Bottom -= (SHORT)(srWindow.Top - Top);
		srWindow.Right -= (SHORT)(srWindow.Left - Left);
		srWindow.Left = (SHORT)Left;
		srWindow.Top = (SHORT)Top;

		SetConsoleWindowInfo(hStdOut, TRUE, &srWindow);
	}

	void Console::SetFontSize(uint32_t Width, uint32_t Height, uint32_t Weight)
	{
		auto hStdOut = ((IO::ConsoleStream*)ConsoleInstance.Out.GetBaseStream())->GetStreamHandle();

		CONSOLE_FONT_INFOEX cFont;
		cFont.cbSize = sizeof(cFont);

		GetCurrentConsoleFontEx(hStdOut, false, &cFont);

		cFont.dwFontSize.X = Width;
		cFont.dwFontSize.Y = Height;
		cFont.FontWeight = Weight;

		SetCurrentConsoleFontEx(hStdOut, false, &cFont);
	}

	void Console::SetTitle(const string& Value)
	{
		SetConsoleTitleA((const char*)Value);
	}

	void Console::SetTitle(const char* Value)
	{
		SetConsoleTitleA(Value);
	}

	void Console::SetCursorVisible(bool Visible)
	{
		auto hStdOut = ((IO::ConsoleStream*)ConsoleInstance.Out.GetBaseStream())->GetStreamHandle();

		CONSOLE_CURSOR_INFO cCI{};
		GetConsoleCursorInfo(hStdOut, &cCI);

		cCI.bVisible = Visible;

		SetConsoleCursorInfo(hStdOut, &cCI);
	}

	void Console::MoveBufferArea(uint32_t SourceLeft, uint32_t SourceTop, uint32_t SourceWidth, uint32_t SourceHeight, uint32_t TargetLeft, uint32_t TargetTop, char SourceChar, ConsoleColor SourceForeColor, ConsoleColor SourceBackColor)
	{
		auto hStdOut = ((IO::ConsoleStream*)ConsoleInstance.Out.GetBaseStream())->GetStreamHandle();

		CONSOLE_SCREEN_BUFFER_INFO cSBI{};
		GetConsoleScreenBufferInfo(hStdOut, &cSBI);

		COORD BufferSize = cSBI.dwSize;
		COORD BufferCoord{};
		SMALL_RECT ReadRegion{};

		if (SourceWidth == 0 || SourceHeight == 0)
			return;

		auto CharBuffer = std::make_unique<CHAR_INFO[]>(SourceWidth * SourceHeight);

		BufferSize.X = (SHORT)SourceWidth;
		BufferSize.Y = (SHORT)SourceHeight;
		ReadRegion.Left = (SHORT)SourceLeft;
		ReadRegion.Right = (SHORT)(SourceLeft + SourceWidth - 1);
		ReadRegion.Top = (SHORT)SourceTop;
		ReadRegion.Bottom = (SHORT)(SourceTop + SourceHeight - 1);

		ReadConsoleOutput(hStdOut, CharBuffer.get(), BufferSize, BufferCoord, &ReadRegion);

		COLORREF NativeColor = (Console::ConsoleColorToColorAttribute(SourceBackColor, true) | Console::ConsoleColorToColorAttribute(SourceForeColor, false));
		DWORD nWrite = 0;

		COORD WriteCoord{};
		WriteCoord.X = (SHORT)SourceLeft;

		SHORT Attr = (SHORT)NativeColor;
		for (uint32_t i = SourceTop; i < (SourceTop + SourceHeight); i++)
		{
			WriteCoord.Y = (SHORT)i;

			FillConsoleOutputCharacterA(hStdOut, SourceChar, SourceWidth, WriteCoord, &nWrite);
			FillConsoleOutputAttribute(hStdOut, Attr, SourceWidth, WriteCoord, &nWrite);
		}

		SMALL_RECT WriteRegion{};
		WriteRegion.Left = (SHORT)TargetLeft;
		WriteRegion.Right = (SHORT)(TargetLeft + SourceWidth);
		WriteRegion.Top = (SHORT)TargetTop;
		WriteRegion.Bottom = (SHORT)(TargetTop + SourceHeight);

		WriteConsoleOutput(hStdOut, CharBuffer.get(), BufferSize, BufferCoord, &WriteRegion);
	}

	void Console::SetMaximizeBoxVisible(bool Visible)
	{
		auto hConsole = GetConsoleWindow();
		auto Style = GetWindowLong(hConsole, GWL_STYLE);

		if (Visible)
			Style |= WS_MAXIMIZEBOX;
		else
			Style &= ~WS_MAXIMIZEBOX;

		SetWindowLong(hConsole, GWL_STYLE, Style);
	}

	void Console::SetMinimizeBoxVisible(bool Visible)
	{
		auto hConsole = GetConsoleWindow();
		auto Style = GetWindowLong(hConsole, GWL_STYLE);

		if (Visible)
			Style |= WS_MINIMIZEBOX;
		else
			Style &= ~WS_MINIMIZEBOX;

		SetWindowLong(hConsole, GWL_STYLE, Style);
	}

	void Console::SetWindowResizable(bool Resizable)
	{
		auto hConsole = GetConsoleWindow();
		auto Style = GetWindowLong(hConsole, GWL_STYLE);

		if (Resizable)
			Style |= WS_SIZEBOX;
		else
			Style &= ~WS_SIZEBOX;

		SetWindowLong(hConsole, GWL_STYLE, Style);
	}

	void Console::CenterWindow()
	{
		auto hConsole = GetConsoleWindow();
		auto hMonitor = MonitorFromWindow(hConsole, MONITOR_DEFAULTTONEAREST);

		if (hMonitor)
		{
			MONITORINFO Info;
			Info.cbSize = sizeof(Info);

			if (GetMonitorInfo(hMonitor, &Info))
			{
				RECT cRect{};
				GetWindowRect(hConsole, &cRect);

				auto Width = (cRect.right - cRect.left);
				auto Height = (cRect.bottom - cRect.top);

				auto X = (Info.rcWork.left + Info.rcWork.right) / 2 - Width / 2;
				auto Y = (Info.rcWork.top + Info.rcWork.bottom) / 2 - Height / 2;

				SetWindowPos(hConsole, NULL, X, Y, 0, 0, SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOSIZE);
			}
		}
	}

	void Console::RemapConsoleColor(ConsoleColor Source, uint8_t R, uint8_t G, uint8_t B)
	{
		auto hStdOut = ((IO::ConsoleStream*)ConsoleInstance.Out.GetBaseStream())->GetStreamHandle();

		CONSOLE_SCREEN_BUFFER_INFOEX cBuffer;
		cBuffer.cbSize = sizeof(cBuffer);
		GetConsoleScreenBufferInfoEx(hStdOut, &cBuffer);

		// GetConsoleScreenBufferInfoEx returns one short here, so we keep resizing each time...
		cBuffer.srWindow.Bottom++;
		cBuffer.srWindow.Right++;

		cBuffer.ColorTable[(int8_t)Source] = RGB(R, G, B);

		SetConsoleScreenBufferInfoEx(hStdOut, &cBuffer);
	}

	void Console::RemapAllConsoleColors(std::initializer_list<System::ConsoleColor> Colors)
	{
		static_assert(sizeof(Colors) == 16, "You must specify all 16 colors");

		auto hStdOut = ((IO::ConsoleStream*)ConsoleInstance.Out.GetBaseStream())->GetStreamHandle();

		CONSOLE_SCREEN_BUFFER_INFOEX cBuffer;
		cBuffer.cbSize = sizeof(cBuffer);
		GetConsoleScreenBufferInfoEx(hStdOut, &cBuffer);

		// GetConsoleScreenBufferInfoEx returns one short here, so we keep resizing each time...
		cBuffer.srWindow.Bottom++;
		cBuffer.srWindow.Right++;

		std::memcpy(cBuffer.ColorTable, Colors.begin(), sizeof(uint32_t) * 16);

		SetConsoleScreenBufferInfoEx(hStdOut, &cBuffer);
	}

	string Console::GetTitle()
	{
		char Buffer[2048]{};
		GetConsoleTitleA(Buffer, 2048);	// Honestly it would be too big at 256...

		return string(Buffer);
	}

	bool Console::GetCursorVisible()
	{
		auto hStdOut = ((IO::ConsoleStream*)ConsoleInstance.Out.GetBaseStream())->GetStreamHandle();

		CONSOLE_CURSOR_INFO cCI{};
		GetConsoleCursorInfo(hStdOut, &cCI);

		return cCI.bVisible;
	}

	ConsoleColor Console::GetForegroundColor()
	{
		auto hStdOut = ((IO::ConsoleStream*)ConsoleInstance.Out.GetBaseStream())->GetStreamHandle();

		CONSOLE_SCREEN_BUFFER_INFO cSBI{};
		GetConsoleScreenBufferInfo(hStdOut, &cSBI);

		return Console::ColorAttributeToConsoleColor((int16_t)(cSBI.wAttributes & Console::ForegroundMask));
	}

	ConsoleColor Console::GetBackgroundColor()
	{
		auto hStdOut = ((IO::ConsoleStream*)ConsoleInstance.Out.GetBaseStream())->GetStreamHandle();

		CONSOLE_SCREEN_BUFFER_INFO cSBI{};
		GetConsoleScreenBufferInfo(hStdOut, &cSBI);

		return Console::ColorAttributeToConsoleColor((int16_t)(cSBI.wAttributes & Console::BackgroundMask));
	}

	void Console::Header(const char* Heading, ConsoleColor Color)
	{
		Console::SetColors(Color, ConsoleColor::DarkGray);

		auto sLen = strlen(Heading);

		char HeaderBuffer[21];
		std::memset(HeaderBuffer + 1, ' ', 19);

		HeaderBuffer[0] = '[';
		HeaderBuffer[20] = (char)0;
		HeaderBuffer[sLen + 1] = ']';

		std::memcpy(HeaderBuffer + 1, Heading, sLen);

		ConsoleInstance.Out.Write((const char*)HeaderBuffer);

		Console::ResetColor();

		ConsoleInstance.Out.Write(": ");
	}

	void Console::Progress(const char* Heading, ConsoleColor Color, uint32_t Width, uint32_t Progress)
	{
		Progress = min(Progress, 100);

		ConsoleInstance.Out.Write("\r");
		Console::Header(Heading, Color);

		string Buffer(Width + 2, '\0', false);
		Buffer.Append("[");

		uint32_t Blocks = min((uint32_t)((Progress / 100.f) * Width), Width);
		for (uint32_t i = 0; i < Width; i++)
		{
			if (i < Blocks)
				Buffer.Append("=");
			else if (i == Blocks)
				Buffer.Append(">");
			else
				Buffer.Append(" ");
		}

		Buffer.Append("]");

		ConsoleInstance.Out.Write(Buffer);
	}

	int32_t Console::Read()
	{
		return ConsoleInstance.In.Read();
	}

	ConsoleKeyInfo Console::ReadKey(bool Intercept)
	{
		INPUT_RECORD iRecord{};
		DWORD rRead = 0;

		auto hStdIn = ((IO::ConsoleStream*)ConsoleInstance.In.GetBaseStream())->GetStreamHandle();

		while (true)
		{
			ReadConsoleInputA(hStdIn, &iRecord, 1, &rRead);

			int16_t kCode = iRecord.Event.KeyEvent.wVirtualKeyCode;

			if (!IsKeyDownEvent(iRecord))
				if (kCode != AltVKCode)
					continue;

			char Ch = (char)iRecord.Event.KeyEvent.uChar.AsciiChar;

			if (Ch == 0)
				if (IsModKey(iRecord))
					continue;

			ConsoleKey key = (ConsoleKey)kCode;
			if (IsAltKeyDown(iRecord) && ((key >= ConsoleKey::NumPad0 && key <= ConsoleKey::NumPad9) || (key == ConsoleKey::Clear) || (key == ConsoleKey::Insert) || (key >= ConsoleKey::PageUp && key <= ConsoleKey::DownArrow)))
				continue;

			// We passed all checks
			break;
		}

		// Calculate key status
		auto State = iRecord.Event.KeyEvent.dwControlKeyState;
		bool Shift = (State & (uint32_t)ControlKeyState::ShiftPressed) != 0;
		bool Alt = (State & ((uint32_t)ControlKeyState::LeftAltPressed | (uint32_t)ControlKeyState::RightAltPressed)) != 0;
		bool Control = (State & ((uint32_t)ControlKeyState::LeftCtrlPressed | (uint32_t)ControlKeyState::RightCtrlPressed)) != 0;

		if (!Intercept)
		{
			char iBuffer[] = { iRecord.Event.KeyEvent.uChar.AsciiChar, 0 };
			Console::Write(iBuffer);
		}

		return ConsoleKeyInfo((char)iRecord.Event.KeyEvent.uChar.AsciiChar, (ConsoleKey)iRecord.Event.KeyEvent.wVirtualKeyCode, Shift, Alt, Control);
	}

	string Console::ReadLine()
	{
		return ConsoleInstance.In.ReadLine();
	}

	bool Console::IsKeyDownEvent(INPUT_RECORD iRecord)
	{
		return (iRecord.EventType == KEY_EVENT && iRecord.Event.KeyEvent.bKeyDown);
	}

	bool Console::IsModKey(INPUT_RECORD iRecord)
	{
		int16_t keyCode = iRecord.Event.KeyEvent.wVirtualKeyCode;

		return ((keyCode >= 0x10 && keyCode <= 0x12) || keyCode == 0x14 || keyCode == 0x90 || keyCode == 0x91);
	}

	bool Console::IsAltKeyDown(INPUT_RECORD iRecord)
	{
		return ((iRecord.Event.KeyEvent.dwControlKeyState) & ((uint32_t)ControlKeyState::LeftAltPressed | (uint32_t)ControlKeyState::RightAltPressed)) != 0;
	}

	ConsoleColor Console::ColorAttributeToConsoleColor(int16_t Attribute)
	{
		if ((Attribute & Console::BackgroundMask) != 0)
			Attribute = (int32_t)(((int32_t)Attribute) >> 4);

		return ConsoleColor(Attribute);
	}

	int16_t Console::ConsoleColorToColorAttribute(ConsoleColor Color, bool isBackground)
	{
		auto Result = (int16_t)Color;

		if (isBackground)
			Result = (int16_t)((int32_t)Result << 4);

		return Result;
	}
}
