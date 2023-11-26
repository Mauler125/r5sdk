//=============================================================================//
//
// Purpose: Interface the engine exposes to the game DLL
//
//=============================================================================//

#include "core/stdafx.h"
#include "engine/cmd.h"
#include "clientstate.h"
#include "vengineclient_impl.h"

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
	const static int index = 35;
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	const static int index = 36;
#endif
	return CallVFunc<int>(index, this);
}

//---------------------------------------------------------------------------------
// Purpose: execute client command
// Input  : *thisptr     - 
//          *szCmdString - 
// Output :
//---------------------------------------------------------------------------------
void CEngineClient::_ClientCmd(CEngineClient* thisptr, const char* const szCmdString)
{
	const bool restrictClientCommands = g_pClientState->m_bRestrictClientCommands;
	const int numMarkers = 2;

	if (restrictClientCommands && !Cbuf_HasRoomForExecutionMarkers(numMarkers))
	{
		DevWarning(eDLL_T::CLIENT, "%s: No room for %i execution markers; command \"%s\" ignored\n",
			__FUNCTION__, numMarkers, szCmdString);
		return;
	}

	if (restrictClientCommands)
	{
		Cbuf_AddExecutionMarker(Cbuf_GetCurrentPlayer(), eCmdExecutionMarker_Enable_FCVAR_CLIENTCMD_CAN_EXECUTE);
	}

	Cbuf_AddText(Cbuf_GetCurrentPlayer(), szCmdString, cmd_source_t::kCommandSrcCode);
	Cbuf_AddText(Cbuf_GetCurrentPlayer(), "\n", cmd_source_t::kCommandSrcCode);

	if (restrictClientCommands)
	{
		Cbuf_AddExecutionMarker(Cbuf_GetCurrentPlayer(), eCmdExecutionMarker_Disable_FCVAR_CLIENTCMD_CAN_EXECUTE);
	}
}

void HVEngineClient::Detour(const bool bAttach) const
{
	DetourSetup(&CEngineClient__ClientCmd, &CEngineClient::_ClientCmd, bAttach);
}
