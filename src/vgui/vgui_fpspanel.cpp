//===========================================================================//
//
// Purpose: Framerate indicator panel.
//
// $NoKeywords: $
//===========================================================================//

#include "core/stdafx.h"
#include "tier1/cvar.h"
#include "vgui/vgui_fpspanel.h"
#include "vgui/vgui_debugpanel.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ConVar* HCFPSPanel_Paint(void* thisptr)
{
	g_pOverlay->Update();
	return CFPSPanel_Paint(thisptr);
}

///////////////////////////////////////////////////////////////////////////////
void VFPSPanel::Detour(const bool bAttach) const
{
	DetourSetup(&CFPSPanel_Paint, &HCFPSPanel_Paint, bAttach);
}
