#include "pch.h"
#include "hooks.h"
#include "logsystem.h"

namespace Hooks
{
	CEngineVGui_PaintFn originalCEngineVGui_Paint = nullptr;
}

int Hooks::CEngineVGui_Paint(void* thisptr, int mode)
{
	int result = originalCEngineVGui_Paint(thisptr, mode);

	static void* pCMatSystemSurface = MemoryAddress(0x14D40B3B0).RCast<void*(*)()>();
	static auto fnRenderStart       = MemoryAddress(0x14053EFC0).RCast<void(*)(void*)>();
	static auto fnRenderEnd         = MemoryAddress(0x14053F1B0).RCast<void*(*)()>();

	if (mode == 1 || mode == 2) // Render in main menu and ingame.
	{
		fnRenderStart(pCMatSystemSurface);

		g_LogSystem.Update();

		fnRenderEnd();
	}

	return result;
}