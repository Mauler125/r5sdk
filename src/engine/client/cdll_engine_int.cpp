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
#include "tier1/keyvalues.h"
#include "windows/id3dx.h"
#include "geforce/reflex.h"
#include "vengineclient_impl.h"
#include "cdll_engine_int.h"
#ifndef DEDICATED
#include "materialsystem/cmaterialsystem.h"
#endif // !DEDICATED
/*****************************************************************************/

static CGlobalVarsBase dummyvars(true);
// So stuff that might reference gpGlobals during DLL initialization won't have a NULL pointer.
// Once the engine calls Init on this DLL, this pointer gets assigned to the shared data in the engine
CGlobalVarsBase* gpGlobals = &dummyvars;

int CHLClient::Init(CHLClient* thisptr, CreateInterfaceFn appSystemFactory, CGlobalVarsBase* pGlobals)
{
	gpGlobals = pGlobals;
	return CHLClient__Init(thisptr, appSystemFactory, pGlobals);
}

#ifndef DEDICATED
//-----------------------------------------------------------------------------
// Purpose: pre frame stage notify hook
//-----------------------------------------------------------------------------
void FrameStageNotify_Pre(const ClientFrameStage_t frameStage)
{
	switch (frameStage)
	{
	case ClientFrameStage_t::FRAME_START:
		break;
	case ClientFrameStage_t::FRAME_NET_UPDATE_START:
		break;
	case ClientFrameStage_t::FRAME_NET_UPDATE_POSTDATAUPDATE_START:
		break;
	case ClientFrameStage_t::FRAME_NET_UPDATE_POSTDATAUPDATE_END:
		break;
	case ClientFrameStage_t::FRAME_NET_UPDATE_END:
		break;
	case ClientFrameStage_t::FRAME_RENDER_START:
		break;
	case ClientFrameStage_t::FRAME_RENDER_END:
		break;
	case ClientFrameStage_t::FRAME_NET_FULL_FRAME_UPDATE_ON_REMOVE:
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: post frame stage notify hook
//-----------------------------------------------------------------------------
void FrameStageNotify_Post(const ClientFrameStage_t frameStage)
{
	switch (frameStage)
	{
	case ClientFrameStage_t::FRAME_START:
		break;
	case ClientFrameStage_t::FRAME_NET_UPDATE_START:
		break;
	case ClientFrameStage_t::FRAME_NET_UPDATE_POSTDATAUPDATE_START:
		break;
	case ClientFrameStage_t::FRAME_NET_UPDATE_POSTDATAUPDATE_END:
		break;
	case ClientFrameStage_t::FRAME_NET_UPDATE_END:
		break;
	case ClientFrameStage_t::FRAME_RENDER_START:
		break;
	case ClientFrameStage_t::FRAME_RENDER_END:
		GFX_SetLatencyMarker(D3D11Device(), SIMULATION_END, MaterialSystem()->GetCurrentFrameCount());
		break;
	case ClientFrameStage_t::FRAME_NET_FULL_FRAME_UPDATE_ON_REMOVE:
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHLClient::FrameStageNotify(CHLClient* pHLClient, ClientFrameStage_t frameStage)
{
	FrameStageNotify_Pre(frameStage);
	CHLClient__FrameStageNotify(pHLClient, frameStage);
	FrameStageNotify_Post(frameStage);
}

//-----------------------------------------------------------------------------
// Purpose: Get g_pClientClassHead Pointer for all ClientClasses.
// Input  :
// Output : ClientClass*
//-----------------------------------------------------------------------------
ClientClass* CHLClient::GetAllClasses()
{
	return CHLClient__GetAllClasses();
}
#endif // !DEDICATED

///////////////////////////////////////////////////////////////////////////////
void VDll_Engine_Int::Detour(const bool bAttach) const
{
#ifndef DEDICATED
	DetourSetup(&CHLClient__Init, &CHLClient::Init, bAttach);
	DetourSetup(&CHLClient__FrameStageNotify, &CHLClient::FrameStageNotify, bAttach);
#endif // !DEDICATED
}
