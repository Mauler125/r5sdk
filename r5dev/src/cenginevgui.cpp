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

	static void* g_pMatSystemSurface = MemoryAddress(0x14D40B3B0).RCast<void* (*)()>();
	static auto RenderStart = MemoryAddress(0x14053EFC0).RCast<void(*)(void*)>();
	static auto RenderEnd = MemoryAddress(0x14053F1B0).RCast<void*(*)()>();

	if (mode == 1 || mode == 2)
	{
		RenderStart(g_pMatSystemSurface);

		g_LogSystem.Update();

		RenderEnd();
	}


	return result;
}