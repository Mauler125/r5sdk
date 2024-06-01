#pragma once

void SetConsoleBackgroundColor(COLORREF color);
void FlashConsoleBackground(int nFlashCount, int nFlashInterval, COLORREF color);

bool Console_Init(const bool bAnsiColor);
bool Console_ColorInit();
bool Console_Shutdown();
