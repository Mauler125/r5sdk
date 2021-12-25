#include "core/stdafx.h"
#include "engine/baseclient.h"

//---------------------------------------------------------------------------------
// Purpose: throw away any residual garbage in the channel
//---------------------------------------------------------------------------------
std::int64_t* HCBaseClient_Clear(std::int64_t client)
{
	return CBaseClient_Clear(client);
}

///////////////////////////////////////////////////////////////////////////////
void CBaseClient_Attach()
{
	DetourAttach((LPVOID*)&CBaseClient_Clear, &HCBaseClient_Clear);
}
void CBaseClient_Detach()
{
	DetourDetach((LPVOID*)&CBaseClient_Clear, &HCBaseClient_Clear);
}
