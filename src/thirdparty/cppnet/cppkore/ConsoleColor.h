#pragma once

#include <cstdint>

namespace System
{
	// This represents the colors that can be used for console text foreground and background colors.
	struct ConsoleColor
	{
		union
		{
			uint32_t NativeIndex;
			uint32_t Color;
		};

		constexpr ConsoleColor()
			: NativeIndex(0)
		{
		}

		// Construct a ConsoleColor from the built-in index
		constexpr ConsoleColor(uint32_t Index)
			: NativeIndex(Index)
		{
		}

		// Construct a new ConsoleColor from the given RGB
		constexpr ConsoleColor(uint8_t R, uint8_t G, uint8_t B)
			: Color(((uint32_t)(((uint8_t)(R) | ((uint16_t)((uint8_t)(G)) << 8)) | (((uint32_t)(uint8_t)(B)) << 16))))
		{
		}

		constexpr operator const uint32_t(void) const
		{
			return (uint32_t)NativeIndex;
		}
		constexpr operator const int32_t(void) const
		{
			return (int32_t)NativeIndex;
		}
		constexpr operator const int8_t(void) const
		{
			return (int8_t)NativeIndex;
		}
		constexpr operator const int16_t(void) const
		{
			return (int16_t)NativeIndex;
		}

		static ConsoleColor Black;
		static ConsoleColor DarkBlue;
		static ConsoleColor DarkGreen;
		static ConsoleColor DarkCyan;
		static ConsoleColor DarkRed;
		static ConsoleColor DarkMagenta;
		static ConsoleColor DarkYellow;
		static ConsoleColor Gray;
		static ConsoleColor DarkGray;
		static ConsoleColor Blue;
		static ConsoleColor Green;
		static ConsoleColor Cyan;
		static ConsoleColor Red;
		static ConsoleColor Magenta;
		static ConsoleColor Yellow;
		static ConsoleColor White;
	};

	static_assert(sizeof(ConsoleColor) == 4, "System::ConsoleColor size mismatch, expected 4!");
}