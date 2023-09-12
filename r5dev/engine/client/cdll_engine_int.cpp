//=============================================================================
//
//
//=============================================================================

#include "core/stdafx.h"
/*****************************************************************************/
#include "tier1/cvar.h"
#include "tier0/commandline.h"
#include "engine/net_chan.h"
#include "engine/client/cl_rcon.h"
#include "networksystem/bansystem.h"
#include "vpc/keyvalues.h"
#include "windows/id3dx.h"
#include "geforce/reflex.h"
#include "vengineclient_impl.h"
#include "cdll_engine_int.h"
/*****************************************************************************/

#ifndef DEDICATED
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHLClient::FrameStageNotify(CHLClient* pHLClient, ClientFrameStage_t frameStage)
{
	// Must be performed before the call, before scene starts rendering.
	if (frameStage == ClientFrameStage_t::FRAME_RENDER_START)
		GFX_SetLatencyMarker(D3D11Device(), RENDERSUBMIT_START);

	CHLClient_FrameStageNotify(pHLClient, frameStage);

	// Must be performed after the call, after the scene has been rendered.
	if (frameStage == ClientFrameStage_t::FRAME_RENDER_END)
		GFX_SetLatencyMarker(D3D11Device(), RENDERSUBMIT_END);
}

//-----------------------------------------------------------------------------
// Purpose: Get g_pClientClassHead Pointer for all ClientClasses.
// Input  :
// Output : ClientClass*
//-----------------------------------------------------------------------------
ClientClass* CHLClient::GetAllClasses()
{
	return CHLClient_GetAllClasses();
}
#endif // !DEDICATED

///////////////////////////////////////////////////////////////////////////////
void VDll_Engine_Int::Attach() const
{
#ifndef DEDICATED
	DetourAttach((LPVOID*)&CHLClient_FrameStageNotify, &CHLClient::FrameStageNotify);
#endif // !DEDICATED
}

void VDll_Engine_Int::Detach() const
{
#ifndef DEDICATED
	DetourDetach((LPVOID*)&CHLClient_FrameStageNotify, &CHLClient::FrameStageNotify);
#endif // !DEDICATED
}
