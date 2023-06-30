#pragma once

void SetConsoleBackgroundColor(COLORREF color);
void FlashConsoleBackground(int nFlashCount, int nFlashInterval, COLORREF color);

void Console_Init(const bool bAnsiColor);
void Console_Shutdown();
