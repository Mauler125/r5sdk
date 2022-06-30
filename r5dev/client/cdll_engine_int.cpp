//=============================================================================
//
//
//=============================================================================

#include "core/stdafx.h"
/*****************************************************************************/
#include "tier1/cvar.h"
#include "tier0/commandline.h"
#include "tier1/IConVar.h"
#include "client/vengineclient_impl.h"
#include "client/cdll_engine_int.h"
#include "engine/net_chan.h"
#include "engine/client/cl_rcon.h"
#include "public/include/bansystem.h"
#include "vpc/keyvalues.h"
/*****************************************************************************/

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHLClient::FrameStageNotify(CHLClient* pHLClient, ClientFrameStage_t frameStage)
{
	CHLClient_FrameStageNotify(pHLClient, frameStage);
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

///////////////////////////////////////////////////////////////////////////////
void CHLClient_Attach()
{
	DetourAttach((LPVOID*)&CHLClient_FrameStageNotify, &CHLClient::FrameStageNotify);
}

void CHLClient_Detach()
{
	DetourDetach((LPVOID*)&CHLClient_FrameStageNotify, &CHLClient::FrameStageNotify);
}
