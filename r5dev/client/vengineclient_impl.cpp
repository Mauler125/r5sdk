//=============================================================================//
//
// Purpose: Interface the engine exposes to the game DLL
//
//=============================================================================//

#include "core/stdafx.h"
#include "client/vengineclient_impl.h"
#include "engine/client/clientstate.h"

//---------------------------------------------------------------------------------
// Purpose: define if commands from the server should be restricted or not.
// Input  : bRestricted - 
// Output :
//---------------------------------------------------------------------------------
void CEngineClient::SetRestrictServerCommands(bool bRestricted)
{
	g_pClientState->m_bRestrictServerCommands = bRestricted;
}

//---------------------------------------------------------------------------------
// Purpose: get value for if commands are restricted from servers.
// Input  :
// Output : bool
//---------------------------------------------------------------------------------
bool CEngineClient::GetRestrictServerCommands() const
{
	return g_pClientState->m_bRestrictServerCommands;
}

//---------------------------------------------------------------------------------
// Purpose: define if commands on the client should be restricted or not.
// Input  : bRestricted - 
// Output :
//---------------------------------------------------------------------------------
void CEngineClient::SetRestrictClientCommands(bool bRestricted)
{
	g_pClientState->m_bRestrictClientCommands = bRestricted;
}

//---------------------------------------------------------------------------------
// Purpose: get value for if commands are restricted for clients.
// Input  :
// Output : bool
//---------------------------------------------------------------------------------
bool CEngineClient::GetRestrictClientCommands() const
{
	return g_pClientState->m_bRestrictClientCommands;
}

//---------------------------------------------------------------------------------
// Purpose: get local player
// Input  :
// Output : int
//---------------------------------------------------------------------------------
int CEngineClient::GetLocalPlayer()
{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	const int index = 35;
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	const int index = 36;
#endif
	return CallVFunc<int>(index, this);
}