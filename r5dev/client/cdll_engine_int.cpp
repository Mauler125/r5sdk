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
#include "networksystem/bansystem.h"
#include "vpc/keyvalues.h"
/*****************************************************************************/

#ifndef DEDICATED
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
