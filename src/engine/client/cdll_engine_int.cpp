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
void VDll_Engine_Int::Detour(const bool bAttach) const
{
#ifndef DEDICATED
	DetourSetup(&CHLClient_FrameStageNotify, &CHLClient::FrameStageNotify, bAttach);
#endif // !DEDICATED
}
