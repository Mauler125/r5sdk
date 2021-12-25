#include "core/stdafx.h"
#include "tier0/cvar.h"
#include "vgui/vgui_fpspanel.h"
#include "vgui/CEngineVGui.h"

ConVar* HCFPSPanel_Paint(void* thisptr)
{
	g_pLogSystem.Update();
	return CFPSPanel_Paint(thisptr);
}

void CFPSPanel_Attach()
{
	DetourAttach((LPVOID*)&CFPSPanel_Paint, &HCFPSPanel_Paint);
}

void CFPSPanel_Detach()
{
	DetourDetach((LPVOID*)&CFPSPanel_Paint, &HCFPSPanel_Paint);
}
