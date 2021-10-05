#include "pch.h"
#include "hooks.h"
#include "logsystem.h"

namespace Hooks
{
	CFPSPanel_PaintFn originalCFPSPanel_Paint = nullptr;
}

void Hooks::CFPSPanel_Paint(void* thisptr)
{
	originalCFPSPanel_Paint(thisptr);

	g_LogSystem.Update();
}