#pragma once

#include <cstdint>
#include <cstdio>
#include "StringBase.h"
#include "ConsoleKey.h"
#include "ConsoleColor.h"
#include "__ConsoleInit.h"
#include "ConsoleKeyInfo.h"

namespace System
{
	class Console
	{
	public:
		// Trigger a standard console beep sound
		static void Beep();
		// Trigger a custom console beep sound
		static void Beep(uint32_t Frequency, uint32_t Duration);
		// Clears the console window
		static void Clear();
		// Clears the current console line
		static void ClearLine();
		// Resets the colors
		static void ResetColor();

		// Sets the foreground color
		static void SetForegroundColor(ConsoleColor Color);
		// Sets the background color
		static void SetBackgroundColor(ConsoleColor Color);
		// Sets both colors
		static void SetColors(ConsoleColor Foreground, ConsoleColor Background);

		// Sets the console buffer size
		static void SetBufferSize(uint32_t Width, uint32_t Height);
		// Moves the cursor to the specified position
		static void SetCursorPosition(uint32_t Left, uint32_t Right);
		// Changes the size of the console window
		static void SetWindowSize(uint32_t Width, uint32_t Height);
		// Changes the position of the console window
		static void SetWindowPosition(uint32_t Left, uint32_t Top);
		// Changes the font of the console window
		static void SetFontSize(uint32_t Width, uint32_t Height, uint32_t Weight);
		// Sets the window title
		static void SetTitle(const string& Value);
		// Sets the window title
		static void SetTitle(const char* Value);
		// Sets whether or not the cursor is visible
		static void SetCursorVisible(bool Visible);

		// Moves the specified area in the buffer to another location
		static void MoveBufferArea(uint32_t SourceLeft, uint32_t SourceTop, uint32_t SourceWidth, uint32_t SourceHeight, uint32_t TargetLeft, uint32_t TargetTop, char SourceChar, ConsoleColor SourceForeColor = ConsoleColor::Gray, ConsoleColor SourceBackColor = ConsoleColor::Black);

		// Sets whether or not the maximize box is visible
		static void SetMaximizeBoxVisible(bool Visible);
		// Sets whether or not the minimize box is visible
		static void SetMinimizeBoxVisible(bool Visible);
		// Make the window resizable
		static void SetWindowResizable(bool Resizable);

		// Centers the console window on the current screen
		static void CenterWindow();

		// Reassign an existing console color
		static void RemapConsoleColor(ConsoleColor Source, uint8_t R, uint8_t G, uint8_t B);
		// Reassign all existing console colors
		static void RemapAllConsoleColors(std::initializer_list<System::ConsoleColor> Colors);

		// Gets the window title
		static string GetTitle();
		// Gets whether or not the cursor is visible
		static bool GetCursorVisible();
		// Gets the current foreground color
		static ConsoleColor GetForegroundColor();
		// Gets the current background color
		static ConsoleColor GetBackgroundColor();

		// Writes text to the console
		template<class... TArgs>
		static void Write(const char* Format, TArgs&&... Args);
		// Writes a line to the console
		template<class... TArgs>
		static void WriteLine(const char* Format = nullptr, TArgs&&... Args);

		// Writes a heading to the console
		static void Header(const char* Heading, ConsoleColor Color);
		// Generates a progress bar in the console
		static void Progress(const char* Heading, ConsoleColor Color, uint32_t Width, uint32_t Progress);

		// Read a character
		static int32_t Read();
		// Reads a key
		static ConsoleKeyInfo ReadKey(bool Intercept = false);
		// Reads a line from the console
		static string ReadLine();

	private:

		// Internal routine for IsKeyDown event
		static bool IsKeyDownEvent(INPUT_RECORD iRecord);
		// Internal routine for IsModKey event
		static bool IsModKey(INPUT_RECORD iRecord);
		// Internal routine for IsAltKeyDown event
		static bool IsAltKeyDown(INPUT_RECORD iRecord);

		// Internal routine to convert an internal console color to a ConsoleColor
		static ConsoleColor ColorAttributeToConsoleColor(int16_t Attribute);
		// Internal routine to convert a ConsoleColor to an internal console color
		static int16_t ConsoleColorToColorAttribute(ConsoleColor Color, bool isBackground);

		// Internal console handle
		static __ConsoleInit ConsoleInstance;

		// The alternate virtual key code
		static constexpr int16_t AltVKCode = 0x12;

		// Masks for internal data
		static constexpr int16_t ForegroundMask = 0xf;
		static constexpr int16_t BackgroundMask = 0xf0;
	};

	template<class... TArgs>
	inline void Console::Write(const char* Format, TArgs&&... Args)
	{
		ConsoleInstance.Out.WriteFmt(Format, std::forward<TArgs>(Args)...);
	}

	template<class... TArgs>
	inline void Console::WriteLine(const char* Format, TArgs&&... Args)
	{
		ConsoleInstance.Out.WriteLineFmt(Format, std::forward<TArgs>(Args)...);
	}
}